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
#include <linux/dma-mapping.h>

#define P44_LEDCHAIN_MAJOR      152     /* use a LOCAL/EXPERIMENTAL major for now */
#define P44_LEDCHAIN_MAX_LEDS   1024

#define DEVICE_NAME "ledchain"


// PWM unit
// - register block
#define MT7688_PWM_BASE 0x10005000L
#define MT7688_PWM_SIZE 0x00000210L
// - access to ioremapped PWM area
void __iomem *pwm_base; // set from ioremap()
#define PWM_ADDR(offset) (pwm_base+offset)
#define PWMENABLE        PWM_ADDR(0)
#define PWM_CHAN_OFFS(channel,reg) (0x10+((channel)*0x40)+(reg))
#define PWM_CHAN(channel,reg) PWM_ADDR(PWM_CHAN_OFFS(channel,reg))
#define NUM_PWMS 4
// - PWM channel register offsets
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

// GDMA unit
// - register block
#define MT7688_GDMA_BASE 0x10002800L
#define MT7688_GDMA_SIZE 0x00000250L
// - access to ioremapped GDMA area
void __iomem *gdma_base; // set from ioremap()
#define GDMA_ADDR(offset) (gdma_base+offset)
#define GDMA_UNMASK_INTSTS GDMA_ADDR(0x200)
#define GDMA_DONE_INTSTS GDMA_ADDR(0x204)
#define GDMA_GCT GDMA_ADDR(0x220)
#define GDMA_CHAN(channel,reg) GDMA_ADDR((channel)*0x10+(reg))
// - GDMA channel register offsets
#define GDMA_SA 0x00
#define GDMA_DA 0x04
#define GDMA_CT0 0x08
#define GDMA_CT1 0x0C



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


static int dma_req = 32; // default -1 => PWM_CHANNEL_DEFAULT
module_param(dma_req, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pwm_channel, "DMA request line to try");



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


// FIXME: %%% debug only
#define sh_iowrite32(val, addr) { printk(KERN_INFO DEVICE_NAME": writing 0x%08X := 0x%08X\n", (u32)(addr), (u32)(val)); }

// DMA buffer
static size_t dma_buffer_size;
static dma_addr_t dma_buffer;
static void *dma_buffer_vaddr;

#define DMA_CHANNEL 2 // FIXME: UGLY: this is just a channel that seems to be free normally, but maybe is NOT free!

void update_leds(const char *buff, size_t len)
{
  u32 outmask = 0;
  u32 ledword;
  const u8 *inptr = buff;
  u32 *outbuf;
  u32 *outptr;
  static DEFINE_SPINLOCK(critical);
  unsigned long irqflags;
  u32 longwordsToSend;
  // debug only
  u32 pwm_wave_num;
  u32 pwm_send_wave_num;
  u32 seg_done_status;

  // number of bytes per LED
  int numComponents = rgbw ? 4 : 3;
  // number of LEDs
  int leds = len/numComponents;
  // limit to max
  if (leds>led_count) leds=led_count;
  // generate data into buffer
  outbuf = (u32*)dma_buffer_vaddr; // cpu/virt address for the buffer
  outptr = outbuf; // start at beginning
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
  longwordsToSend = (u32)(outptr-outbuf);
  if (outmask!=0) longwordsToSend++; // Note: outmask==0 means that the longword at outptr was not yet used
  printk(KERN_INFO DEVICE_NAME": longwords to send=%u outmask=0x%08X\n", longwordsToSend, outmask);

  /*
  printk(KERN_INFO DEVICE_NAME": SEND0=0x%08X SEND1=0x%08X\n", outbuf[0], outbuf[1]);

  printk(KERN_INFO DEVICE_NAME": PWMENABLE=0x%08X\n", (u32) PWMENABLE);
  printk(KERN_INFO DEVICE_NAME": PWMCON=0x%08X\n", (u32) PWM_CHAN(pwm_channel, PWMCON));
  printk(KERN_INFO DEVICE_NAME": PWMSENDDATA0=0x%08X\n", (u32) PWM_CHAN(pwm_channel, PWMSENDDATA0));

  printk(KERN_INFO DEVICE_NAME": current PWMENABLE=0x%08X\n", (u32) ioread32(PWMENABLE));

  // - disable the PWM
  sh_iowrite32(ioread32(PWMENABLE) & ~(1<<pwm_channel), PWMENABLE); // disable PWM
  // - set up PWM for one output sequence
  sh_iowrite32(0x7E08, PWM_CHAN(pwm_channel, PWMCON)); // PWMxCON: New PWM mode, all 64 bits, idle=0, guard=0, 40Mhz clock, no clock dividing
  sh_iowrite32(14, PWM_CHAN(pwm_channel, inverted ? PWMLDUR : PWMHDUR)); // high time is 350nS (25nS/clk@40Mhz)
  sh_iowrite32(32, PWM_CHAN(pwm_channel, inverted ? PWMHDUR : PWMLDUR)); // low time is 800nS (25nS/clk@40Mhz)
  sh_iowrite32(0, PWM_CHAN(pwm_channel, PWMGDUR)); // no guard time
  sh_iowrite32(1, PWM_CHAN(pwm_channel, PWMWAVENUM)); // one single wave
  sh_iowrite32(outbuf[0], PWM_CHAN(pwm_channel, PWMSENDDATA0)); // Upper 32 bits // FIXME: deliver these by DMA later!
  sh_iowrite32(outbuf[1], PWM_CHAN(pwm_channel, PWMSENDDATA1)); // Lower 32 bits // FIXME: deliver these by DMA later!
  // - enable the PWM
  sh_iowrite32(ioread32(PWMENABLE) | (1<<pwm_channel), PWMENABLE); // enable PWM
  */

  // pass buffer to PWM
  spin_lock_irqsave(&critical, irqflags);
  // get some info
  pwm_wave_num = ioread32(PWM_CHAN(pwm_channel, PWMWAVENUM));
  pwm_send_wave_num = ioread32(PWM_CHAN(pwm_channel, PWMSENDWAVENUM));
  seg_done_status = ioread32(GDMA_DONE_INTSTS);

  // prepare the DMA
  // - disable the PWM
  iowrite32(ioread32(PWMENABLE) & ~(1<<pwm_channel), PWMENABLE); // disable PWM
  // - set up PWM for one output sequence
  iowrite32(0x7E08, PWM_CHAN(pwm_channel, PWMCON)); // PWMxCON: New PWM mode, all 64 bits, idle=0, guard=0, 40Mhz clock, no clock dividing
  iowrite32(14, PWM_CHAN(pwm_channel, inverted ? PWMLDUR : PWMHDUR)); // high time is 350nS (25nS/clk@40Mhz)
  iowrite32(40, PWM_CHAN(pwm_channel, inverted ? PWMHDUR : PWMLDUR)); // low time is 1000nS (25nS/clk@40Mhz)
  iowrite32(0, PWM_CHAN(pwm_channel, PWMGDUR)); // no guard time
  iowrite32(1, PWM_CHAN(pwm_channel, PWMWAVENUM)); // one single wave
  // - prepare data source
  if (longwordsToSend>2) {
		// start dma transfer
		// - Source address of GDMA channel (our buffer), as physical address
		iowrite32((u32)dma_buffer, GDMA_CHAN(DMA_CHANNEL, GDMA_SA));
    // - Destination address of GDMA channel (PWMSENDDATA0), as physical address
		iowrite32(MT7688_PWM_BASE+PWM_CHAN_OFFS(pwm_channel, PWMSENDDATA0), GDMA_CHAN(DMA_CHANNEL, GDMA_DA));
    // Control Register 1 of GDMA channel:
    // - number of segments/2 = 0
    // - source DMA request = 32 = from memory
    // - continuous mode disabled
    // - destination DMA request = bits8..13
    // - do not clear another channel's CH_MASK -> set to own channel = 2
    // - no extra read of destination for coherency (is for memory dest only!)
    // - no CH_UNMASK fail interrupt
    // - CH_MASK = 0 (not gated)
		iowrite32(0x00200010 | (dma_req<<8), GDMA_CHAN(DMA_CHANNEL, GDMA_CT1));
    // Control Register 0 of GDMA channel 2:
    // - target byte count = 4*longwordsToSend, bits 16..31
    // - source address mode = 0 = incrementing
    // - destination address mode = 1 = fixed
    // - burst size = 000 -> one doubleword (=32bits)
    // - DISABLE segment done interrupt
    // - CH_EN = 1 -> channel enabled (will be cleared when segment is done)
    // - SW_MODE_EN = 0 -> hardware mode: transfer starts when DMA request is asserted
		iowrite32(0x0042+(longwordsToSend<<18), GDMA_CHAN(DMA_CHANNEL, GDMA_CT0));
  }
  else {
    // just send data directly
    iowrite32(outbuf[0], PWM_CHAN(pwm_channel, PWMSENDDATA0)); // Upper 32 bits
    iowrite32(outbuf[1], PWM_CHAN(pwm_channel, PWMSENDDATA1)); // Lower 32 bits
  }
  // - enable the PWM
  iowrite32(ioread32(PWMENABLE) | (1<<pwm_channel), PWMENABLE); // enable PWM
  spin_unlock_irqrestore(&critical, irqflags);
  // FIXME: remove debug code
  printk(KERN_INFO DEVICE_NAME": before sending, we had pwm_wave_num=%u, pwm_send_wave_num=%u, seg_done_status=0x%08X\n", pwm_wave_num, pwm_send_wave_num, seg_done_status);
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
static struct device *p44ledchain_dev;

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
  p44ledchain_dev = device_create(p44ledchain_class, NULL, MKDEV(P44_LEDCHAIN_MAJOR, 0), NULL, DEVICE_NAME);
  if (!p44ledchain_dev) {
    printk(KERN_ALERT DEVICE_NAME": device_create failed. Try 'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, P44_LEDCHAIN_MAJOR);
    return -ENXIO;
  }

  // map PWM registers
  // TODO: should also do request_mem_region()
  pwm_base = ioremap(MT7688_PWM_BASE, MT7688_PWM_SIZE);
  printk(KERN_INFO DEVICE_NAME": pwm_base=0x%08X\n", (u32)pwm_base);

  // map GDMA registers
  // TODO: should also do request_mem_region()
  gdma_base = ioremap(MT7688_GDMA_BASE, MT7688_GDMA_SIZE);
  printk(KERN_INFO DEVICE_NAME": gdma_base=0x%08X\n", (u32)gdma_base);

  // allocate a DMA buffer
  // - one LED bit needs 2 or 3 PWM bits
  // - one LED has 3 or 4 bytes
  dma_buffer_size = led_count*4*3+3; // max number of bytes required for full chain, rounded to next longword
	dma_buffer_vaddr = dma_alloc_coherent(p44ledchain_dev, dma_buffer_size, &dma_buffer, GFP_DMA);
  if (!dma_buffer_vaddr) {
    printk(KERN_ALERT DEVICE_NAME": Error: cannot get coherent DMA buffer of size %d\n", dma_buffer_size);
    return -EINVAL;
  }
  printk(KERN_INFO DEVICE_NAME": DMA buffer, size=%d, virtual address=0x%08X, physical address=0x%08X\n", dma_buffer_size, (u32)dma_buffer_vaddr, (u32)dma_buffer);

  return SUCCESS;
}


/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
  // return DMA buffer
  if (dma_buffer_vaddr) {
  	dma_free_coherent(p44ledchain_dev, dma_buffer_size, dma_buffer_vaddr, dma_buffer);
  	dma_buffer_vaddr = NULL;
  }

  // unmap GDMA
  iounmap(gdma_base);
  // unmap PWM
  iounmap(pwm_base);

  device_destroy(p44ledchain_class, MKDEV(P44_LEDCHAIN_MAJOR,0));
  class_destroy(p44ledchain_class);
  unregister_chrdev(P44_LEDCHAIN_MAJOR, DEVICE_NAME);
  printk(KERN_ALERT DEVICE_NAME": cleaned up\n");
}






/*
 * Methods
 */

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/ledchain"
 */
static int device_open(struct inode *inode, struct file *file)
{
  static int opencount = 0;

  if (Device_Open)
    return -EBUSY;
  Device_Open++;
  sprintf(msg, "device_open called for /dev/%s now %d times\n", DEVICE_NAME, opencount++);
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
static ssize_t device_read(
  struct file *filp, /* see include/linux/fs.h   */
  char *buffer,  /* buffer to fill with data */
  size_t length, /* length of the buffer     */
  loff_t * offset
) {
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
