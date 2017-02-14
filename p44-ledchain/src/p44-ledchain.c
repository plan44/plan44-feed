/*
 *  p44-ledchain.c - A PWM/DMA kernel module for MT7688 SoC to control serial LEDs (WS28xx, SK68xx)
 *
 *  Copyright (C) 2017 Lukas Zeller <luz@plan44.ch> (RGBW support)
 *  Completely rewritten, but made compatible with and partially based on code snippets from
 *  ws2812-draiveris bitbanging driver for AR9931 SoC by Žilvinas Valinskas, Saulius Lukšė, Jürgen Weigert
 *
 *  This is free software, licensed under the GNU General Public License v2.
 *  See /LICENSE for more information.
 *
 * parameters:
 *      pwm_channel=1           # set the PWM channel to use, default = 1
 *      led_count=1             # number of LEDs in the chain, default = 1
 *      inverted=0              # have inverting line drivers at the GPIOs, default = 0
 *      rgbw=0                  # set 1 for quad-channel RGBW LEDs (SK6812), default = 0
 *
 */

#include <linux/init.h>
#include <linux/module.h>       /* Needed by all modules */
#include <linux/kernel.h>       /* Needed for KERN_INFO */
#include <linux/moduleparam.h>
#include <linux/stat.h>

#include <linux/types.h>
#include <linux/string.h>

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/watchdog.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>  /* for put_user */
#include <linux/fs.h>

#include <linux/dmaengine.h>

#define P44_LEDCHAIN_MAJOR      152     /* use a LOCAL/EXPERIMENTAL major for now */
#define P44_LEDCHAIN_MAX_LEDS   1024

#define DEVICE_NAME "ledchain"


// FROM MT7688 datasheet
// - the PWM unit
#define MT7688_PWM_BASE 0x10005000L
#define MT7688_PWM_SIZE 0x10000210L

// - access to ioremapped PWM area
void __iomem *pwm_base; // set from ioremap()
#define PWM_ADDR(offset) (pwm_base+offset)
#define PWMENABLE        PWM_ADDR(0)
#define PWM(channel,reg) PWM_ADDR(0x10+((channel)*0x40)+(reg))
#define NUM_PWMS 4

#define PWMCON			    0x00
#define PWMHDUR			    0x04
#define PWMLDUR			    0x08
#define PWMGDUR			    0x0c
#define PWMSENDDATA0	  0x20
#define PWMSENDDATA1	  0x24
#define PWMWAVENUM	    0x28
#define PWMDWIDTH		    0x2c
#define PWMTHRES		    0x30
#define PWMSENDWAVENUM  0x34

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lukas Zeller luz@plan44.ch");
MODULE_DESCRIPTION("PWM/DMA driver for WS281x and SK68xx led chains");
MODULE_ALIAS_CHARDEV_MAJOR(P44_LEDCHAIN_MAJOR);


static int pwm_channel = 1; // default -1 => PWM_CHANNEL_DEFAULT
module_param(pwm_channel, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pwm_channel, "PWM channel to use. Default is 1");

static int inverted = 0; // default is 0 == normal. 1 == inverted is good for 74HCT02 line drivers.
module_param(inverted, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(inverted, "drive inverted outputs");

static int rgbw = 0; // default is 0 == WS21812. 1 == SK6812 RGBW LEDs
module_param(rgbw, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(rgbw, "RGBW LEDs (SK6812)");

static int led_count = 1; // default is 1, one single LED
module_param(led_count, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(led_count, "number of LEDs");



void generateBit(int aBit, u32 **aOutbuf, u32 *aOutMask)
{
  u32 *op = *aOutbuf;
  u32 om = *aOutMask;
  if (om==0) {
    om=1L<<31; // fill MSB first
    *op = 0; // init next buffer word
  }
  if (aBit!=inverted) {
    // set output bit high
    *op = *op | om;
  }
  else {
    // set output bit low
    *op = *op & ~om;
  }
  // next bit
  om = om >> 1;
  if (om==0) {
    // begin next longword
    *aOutbuf = op+1;
  }
  *aOutMask = om;
}


// generate bit pattern to be fed into PWM engine:
// one input high bit generates a 110 sequence, one input low bit generates a 100 sequence
void generateBits(u32 aWord, u8 aNumBits, u32 **aOutbuf, u32 *aOutMask)
{
  u32 inMask = 1L<<31;
  int bit;
  while (aNumBits>0) {
    // generate next bit
    bit = (aWord & inMask) != 0;
    generateBit(1, aOutbuf, aOutMask); // first bit always high
    if (bit) generateBit(1, aOutbuf, aOutMask); // generate a second high period for a high input bit
//    generateBit(bit, aOutbuf, aOutMask); // middle bit depends on input data bit
    generateBit(0, aOutbuf, aOutMask); // last bit always low
    // shift input bit mask to next bit
    inMask = inMask>>1;
    // bit done
    aNumBits--;
  }
}


#define sh_iowrite32(val, addr) { printk(KERN_INFO DEVICE_NAME": writing 0x%08X := 0x%08X\n", (u32)(addr), (u32)(val)); }

void update_leds(const char *buff, size_t len)
{
  // FIXME: real DMA buffer later
  u32 outbuf[2]; // SEND0 and SEND1 for now
  u32 outmask = 0;
  u32 ledword;
  const u8 *inptr = buff;
  u32 *outptr;
  static DEFINE_SPINLOCK(critical);
  unsigned long irqflags;

  // number of bytes per LED
  int numComponents = rgbw ? 4 : 3;
  // number of LEDs
  int leds = len/numComponents;
  // limit to max
  if (leds>led_count) leds=led_count;
  // generate data into buffer
  outptr = &outbuf[0];
  // FIXME: set DMA for this later
  leds = 1; // FIXME: only one LED for now, we can only send 64 bits for now
  // generate bits into buffer
  while (leds>0) {
    if (rgbw) {
      // rgbw -> grbw
      ledword =
        inptr[1]<<24 |
        inptr[0]<<16 |
        inptr[2]<<8 |
        inptr[3];
      generateBits(ledword, 32, &outptr, &outmask);
      inptr += 4;
    }
    else {
      // 0rgb -> grb
      ledword =
        inptr[1]<<24 |
        inptr[0]<<16 |
        inptr[2]<<8;
      generateBits(ledword, 24, &outptr, &outmask);
      inptr += 3;
    }
    // next LED
    leds--;
  }
  printk(KERN_INFO DEVICE_NAME": outptr-outbuf=%u outmask=0x%08X\n", (u32)(outptr-outbuf), outmask);

  printk(KERN_INFO DEVICE_NAME": SEND0=0x%08X SEND1=0x%08X\n", outbuf[0], outbuf[1]);

  printk(KERN_INFO DEVICE_NAME": PWMENABLE=0x%08X\n", (u32) PWMENABLE);
  printk(KERN_INFO DEVICE_NAME": PWMCON=0x%08X\n", (u32) PWM(pwm_channel, PWMCON));
  printk(KERN_INFO DEVICE_NAME": PWMSENDDATA0=0x%08X\n", (u32) PWM(pwm_channel, PWMSENDDATA0));

  printk(KERN_INFO DEVICE_NAME": current PWMENABLE=0x%08X\n", (u32) ioread32(PWMENABLE));

  // - disable the PWM
  sh_iowrite32(ioread32(PWMENABLE) & ~(1<<pwm_channel), PWMENABLE); // disable PWM
  // - set up PWM for one output sequence
  sh_iowrite32(0x7E08, PWM(pwm_channel, PWMCON)); // PWMxCON: New PWM mode, all 64 bits, idle=0, guard=0, 40Mhz clock, no clock dividing
  sh_iowrite32(14, PWM(pwm_channel, inverted ? PWMLDUR : PWMHDUR)); // high time is 350nS (25nS/clk@40Mhz)
  sh_iowrite32(32, PWM(pwm_channel, inverted ? PWMHDUR : PWMLDUR)); // low time is 800nS (25nS/clk@40Mhz)
  sh_iowrite32(0, PWM(pwm_channel, PWMGDUR)); // no guard time
  sh_iowrite32(1, PWM(pwm_channel, PWMWAVENUM)); // one single wave
  sh_iowrite32(outbuf[0], PWM(pwm_channel, PWMSENDDATA0)); // Upper 32 bits // FIXME: deliver these by DMA later!
  sh_iowrite32(outbuf[1], PWM(pwm_channel, PWMSENDDATA1)); // Lower 32 bits // FIXME: deliver these by DMA later!
  // - enable the PWM
  sh_iowrite32(ioread32(PWMENABLE) | (1<<pwm_channel), PWMENABLE); // enable PWM

  // pass buffer to PWM
  spin_lock_irqsave(&critical, irqflags);
  // FIXME: for now, single LED only to test bit generator
  // - disable the PWM
  iowrite32(ioread32(PWMENABLE) & ~(1<<pwm_channel), PWMENABLE); // disable PWM
  // - set up PWM for one output sequence
  iowrite32(0x7E08, PWM(pwm_channel, PWMCON)); // PWMxCON: New PWM mode, all 64 bits, idle=0, guard=0, 40Mhz clock, no clock dividing
  iowrite32(14, PWM(pwm_channel, inverted ? PWMLDUR : PWMHDUR)); // high time is 350nS (25nS/clk@40Mhz)
  iowrite32(32, PWM(pwm_channel, inverted ? PWMHDUR : PWMLDUR)); // low time is 800nS (25nS/clk@40Mhz)
  iowrite32(0, PWM(pwm_channel, PWMGDUR)); // no guard time
  iowrite32(1, PWM(pwm_channel, PWMWAVENUM)); // one single wave
  iowrite32(outbuf[0], PWM(pwm_channel, PWMSENDDATA0)); // Upper 32 bits // FIXME: deliver these by DMA later!
  iowrite32(outbuf[1], PWM(pwm_channel, PWMSENDDATA1)); // Lower 32 bits // FIXME: deliver these by DMA later!
  // - enable the PWM
  iowrite32(ioread32(PWMENABLE) | (1<<pwm_channel), PWMENABLE); // enable PWM
  spin_unlock_irqrestore(&critical, irqflags);
}


/*
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0

/*
 * Global variables are declared as static, so are global within the file.
 */

static int Device_Open = 0; /* Is device open?
         * Used to prevent multiple access to device */
#define BUF_LEN 100    /* Max length of the message from the device = hello message */
static char msg[BUF_LEN]; /* The msg the device will give when asked */
static char *msg_Ptr;

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};

// we must create *and remove* device classes properly, otherwise we get horrible
// kernel stack backtraces.
static struct class *p44ledchain_class;

int init_module(void)
{
  int r;

  if ((r = register_chrdev(P44_LEDCHAIN_MAJOR, DEVICE_NAME, &fops))) {
    printk(KERN_ALERT DEVICE_NAME": Registering char device major=%d failed with %d\n", P44_LEDCHAIN_MAJOR, r);
    return r;
  }
  if (pwm_channel >= NUM_PWMS) {
    printk(KERN_ALERT DEVICE_NAME": Error: pwm_channel=%d > MAX=%d\n", pwm_channel, NUM_PWMS-1);
    return -EINVAL;
  }
  if (led_count >= P44_LEDCHAIN_MAX_LEDS) {
    printk(KERN_ALERT DEVICE_NAME": Error: led_count=%d > MAX=%d\n", led_count, P44_LEDCHAIN_MAX_LEDS-1);
    return -EINVAL;
  }

  // printk(KERN_INFO DEVICE_NAME": Build: %s %s\n", __DATE__, __TIME__); // do not use __DATE__/__TIME__ -> prevents reproducible builds
  printk(KERN_INFO DEVICE_NAME": Major=%d  device=/dev/%s\n", P44_LEDCHAIN_MAJOR, DEVICE_NAME);
  printk(KERN_INFO DEVICE_NAME": PWM channel=%d\n", pwm_channel);
  printk(KERN_INFO DEVICE_NAME": Number of LEDs: %d\n", led_count);
  printk(KERN_INFO DEVICE_NAME": Inverted: %d\n", inverted);
  printk(KERN_INFO DEVICE_NAME": RGBW LEDs: %d\n", rgbw);

  p44ledchain_class = class_create(THIS_MODULE, DEVICE_NAME);
  if (!device_create(p44ledchain_class, NULL, MKDEV(P44_LEDCHAIN_MAJOR, 0), NULL, DEVICE_NAME ))
    {
      printk(KERN_ALERT DEVICE_NAME": device_create failed. Try 'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, P44_LEDCHAIN_MAJOR);
      // return -ENXIO;         // better continue anyway...
    }

  // map PWM
  // TODO: should also do request_mem_region()
  pwm_base = ioremap(MT7688_PWM_BASE, MT7688_PWM_SIZE);
  printk(KERN_INFO DEVICE_NAME": pwm_base=0x%08X\n", (u32)pwm_base);

  // TODO: get a DMA buffer and a DMA channel

  return SUCCESS;
}


/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
  // unmap PWM
  iounmap(pwm_base);

  // if we don't survive this, sysfs remains in a broken state.
  // You will see later: sysfs: cannot create duplicate filename '/class/ws2812'
  device_destroy(p44ledchain_class, MKDEV(P44_LEDCHAIN_MAJOR,0));
  class_destroy(p44ledchain_class);
  unregister_chrdev(P44_LEDCHAIN_MAJOR, DEVICE_NAME);
  printk(KERN_ALERT DEVICE_NAME": Bye, that's all.\n");
}






/*
 * Methods
 */

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
  static int counter = 0;

  if (Device_Open)
    return -EBUSY;

  Device_Open++;
  sprintf(msg, "I already told you %d times Hello world!\n", counter++);
  msg_Ptr = msg;
  try_module_get(THIS_MODULE);

  return SUCCESS;
}

/*
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
  Device_Open--;    /* We're now ready for our next caller */

  /*
   * Decrement the usage count, or else once you opened the file, you'll
   * never get get rid of the module.
   */
  module_put(THIS_MODULE);

  return 0;
}

/*
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */
         char *buffer,  /* buffer to fill with data */
         size_t length, /* length of the buffer     */
         loff_t * offset)
{
  /*
   * Number of bytes actually written to the buffer
   */
  int bytes_read = 0;

  /*
   * If we're at the end of the message,
   * return 0 signifying end of file
   */
  if (*msg_Ptr == 0)
    return 0;

  /*
   * Actually put the data into the buffer
   */
  while (length && *msg_Ptr) {

    /*
     * The buffer is in the user data segment, not the kernel
     * segment so "*" assignment won't work.  We have to use
     * put_user which copies data from the kernel data segment to
     * the user data segment.
     */
    put_user(*(msg_Ptr++), buffer++);

    length--;
    bytes_read++;
  }

  /*
   * Most read functions return the number of bytes put into the buffer
   */
  return bytes_read;
}


// To reach all connected leds send: len = gpio_count * leds_per_gpio * 3 bytes (4 bytes in rgbw mode)
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
  update_leds(buff, len);
  return len;
}
