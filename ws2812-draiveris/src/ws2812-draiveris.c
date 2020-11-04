/*
 *  ws2812-draiveris.c - A bitbang kernel module for ws2812
 *
 *  Copyright (C) 2006-2012 OpenWrt.org
 *  Copyright (C) 2012 Žilvinas Valinskas, Saulius Lukšė
 *  Copyright (C) 2014 Jürgen Weigert <jw@owncloud.com>
 *  Copyright (C) 2016 Lukas Zeller <luz@plan44.ch> (RGBW support)
 *
 *  This is free software, licensed under the GNU General Public License v2.
 *  See /LICENSE for more information.
 *
 * parameters:
 *      gpio_number=20          # set base number.
 *      gpio_count=3            # default: 1, enable multiple outputs, counting from gpio_number.
 *      gpios=20,21,22          # explicitly specify where the led_strips are connected.
 *      leds_per_gpio           # length of the led strips.
 *      inverted=0              # have inverting line drivers at the GPIOs
 *      rgbw=0                  # set 1 for quad-channel RGBW LEDs (SK6812)
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
#include <linux/uaccess.h>  /* for put_user */
#include <linux/fs.h>

#define LED_STRIPS_SEQUENTIAL   1       /* 0: PARALLEL: one set of values for all outputs */
#define WS2812_MAJOR            152     /* use a LOCAL/EXPERIMENTAL major for now */

#define sysRegRead(phys)         (*(volatile u32 *)KSEG1ADDR(phys))
#define sysRegWrite(phys, val)  ((*(volatile u32 *)KSEG1ADDR(phys)) = (val))

#if defined(CONFIG_ATH79)

// FROM http://www.eeboard.com/wp-content/uploads/downloads/2013/08/AR9331.pdf p65, p77
#define SYS_REG_GPIO_OE         0x18040000L
#define SYS_REG_GPIO_IN         0x18040004L
#define SYS_REG_GPIO_OUT        0x18040008L
#define SYS_REG_GPIO_SET        0x1804000CL
#define SYS_REG_GPIO_CLEAR      0x18040010L
#define SYS_REG_GPIO_INT        0x18040014L
#define SYS_REG_GPIO_INT_TYPE   0x18040018L
#define SYS_REG_GPIO_INT_POLARITY       0x1804001CL
#define SYS_REG_GPIO_INT_PENDING        0x18040020L
#define SYS_REG_GPIO_INT_MASK           0x18040024L
#define SYS_REG_GPIO_INT_FUNCTION_1     0x18040028L
#define SYS_REG_GPIO_IN_ETH_SWITCH_KED  0x1804002CL
#define SYS_REG_GPIO_INT_FUNCTION_2     0x18040030L

#define SYS_REG_RST_WATCHDOG_TIMER 0x1806000CL

#elif defined(CONFIG_RALINK)

#define SYS_REG_GPIO_OE         0xb0000600L
#define SYS_REG_GPIO_SET        0xb0000630L
#define SYS_REG_GPIO_CLEAR      0xb0000640L

#else
#error "NO SOC DEFINED"
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Saulius Lukse saulius.lukse@gmail.com; Juergen Weigert juewei@fabfolk.com");
MODULE_DESCRIPTION("Bitbang GPIO driver for multiple WS2812 led chains");
MODULE_ALIAS_CHARDEV_MAJOR(WS2812_MAJOR);


#define GPIO_NUMBER_DEFAULT 20
static int gpio_number = -1; // default -1 => GPIO_NUMBER_DEFAULT
module_param(gpio_number, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(gpio_number, "GPIO number of first chain. Either use 'gpio_number=FIRST gpio_count=COUNT' or use 'gpios=FIRST,SECOND,THIRD,...'.");

static int inverted = 0; // default is 0 == normal. 1 == inverted is good for 74HCT02 line drivers.
module_param(inverted, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(inverted, "drive inverted outputs");

static int rgbw = 0; // default is 0 == WS21812. 1 == SK6812 RGBW LEDs
module_param(rgbw, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(rgbw, "RGBW LEDs (SK6812)");

static int gpio_count = 1; // default is 3 == the board, I currently have.
module_param(gpio_count, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(gpio_count, "use additional GPIOs if > 1.");

static int leds_per_gpio = 90; // only used when gpio_count > 1.
module_param(leds_per_gpio, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(leds_per_gpio, "start of next led chain");

static char *gpios = NULL;
module_param(gpios, charp, 0);
MODULE_PARM_DESC(gpios, "Comma separated list of GPIO numbers. Either use this, or 'gpio_number= gpio_count='.");

#define SET_GPIOS_H(gpio_bits)  sysRegWrite(SYS_REG_GPIO_SET, gpio_bits)
#define SET_GPIOS_L(gpio_bits)  sysRegWrite(SYS_REG_GPIO_CLEAR, gpio_bits)

#define GPIO_LIST_MAX   8       // not sure what a realistic limit is.
static u_int32_t gpio_list[GPIO_LIST_MAX];
static u_int32_t gpio_bit[GPIO_LIST_MAX];
static int gpio_bit_mask;

// led_bits_m_inverted() sends a pulse to multiple GPIO pins at once. Inverted output.
// The gpio_early_mask indicates which GPIO see a short pulse (aka low value).
// The gpio_early_mask must be a subset of gpio_bit_mask.
void led_bits_m_inverted(u_int32_t gpio_early_mask)
{
    // High value patterns:
    //  good: 12L3H .. 12L8H 11L4H .. 16L4H
    //  bad:  12L2H 10L4H
    //  best: 12L4H (shortest, with one safety each)

    // Low value patterns:
    //  good: 3L12H .. 5L12H 4L11H
    //  bad: 2L12H 8L12H 6L12H 4L8H 4L10H 3L11H
    //  almost: 4L12H has sporadic green flashes on the first led.
    //  best: 3L12H (shortest, with one safety each)

    // Tested with LED_STRIPS_SEQUENTIAL
    // good: 3/9/4 3/8/4
    // almost: 3/7/4    sporadic flicker at led 70..90 of first chain.
    // almost: 3/8/3    sporadic flicker at led 50..90 of all chains.
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_H(gpio_early_mask);

    SET_GPIOS_H(gpio_early_mask);
    SET_GPIOS_H(gpio_early_mask);
    SET_GPIOS_H(gpio_early_mask);
    SET_GPIOS_H(gpio_early_mask);

    SET_GPIOS_H(gpio_early_mask);
    SET_GPIOS_H(gpio_early_mask);
    SET_GPIOS_H(gpio_early_mask);

    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
}

// led_bits_m() sends a pulse to multiple GPIO pins at once. Non-inverted output.
// The gpio_early_mask indicates which GPIO see a short pulse (aka low value).
// The gpio_early_mask must be a subset of gpio_bit_mask.
void led_bits_m(u_int32_t gpio_early_mask)
{
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_H(gpio_bit_mask);
    SET_GPIOS_L(gpio_early_mask);

    SET_GPIOS_L(gpio_early_mask);
    SET_GPIOS_L(gpio_early_mask);
    SET_GPIOS_L(gpio_early_mask);
    SET_GPIOS_L(gpio_early_mask);

    SET_GPIOS_L(gpio_early_mask);
    SET_GPIOS_L(gpio_early_mask);
    SET_GPIOS_L(gpio_early_mask);
    SET_GPIOS_L(gpio_early_mask);

    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
    SET_GPIOS_L(gpio_bit_mask);
}

void update_leds(const char *buff, size_t len)
{
  unsigned long flags;
  long int i = 0;
  int gpio_used = gpio_count;
  int stride = 0;
  static int grb_increment[3+1] = { -1, 2, 2, /* start */ 1 };        // swap red and green: r g b -> g r b, starting at offset 1
  static int grbw_increment[4+1] = { -1, 2, 1, 2, /* start */ 1 };    // swap red and green: r g b w -> g r b w, starting at offset 1
  int component_idx = 0;                                // component index within single LED
  int numComponents = 0;
  int *incrementTable = NULL;

  static DEFINE_SPINLOCK(critical);

  // derive mode dependent parameters
  if (rgbw) {
    numComponents = 4;
    incrementTable = grbw_increment;
  }
  else {
    numComponents = 3;
    incrementTable = grb_increment;
  }
  stride = numComponents * leds_per_gpio;

  if (stride > len) stride = len;
  if (len < gpio_used * stride) gpio_used = (int)(len/stride)+1;

#if defined(CONFIG_ATH79)
  sysRegWrite(SYS_REG_RST_WATCHDOG_TIMER, 1<<31); // Just in case set watchdog to timeout some time later
#endif

  // http://www.eeboard.com/wp-content/uploads/downloads/2013/08/AR9331.pdf
  // p65: SYS_REG_GPIO_OE 0: Enables the driver to be used as input mechanism; 1: Enables the output driver
  i = sysRegRead(SYS_REG_GPIO_OE);
  sysRegWrite(SYS_REG_GPIO_OE, i|gpio_bit_mask);        // output enable to PIN20

  if (inverted) {

      // wait 65uS
      for(i=0;i<1000;i++)
      {
        volatile int j = i;
        SET_GPIOS_H(gpio_bit_mask);
      }


      spin_lock_irqsave(&critical, flags);
      for (i = incrementTable[numComponents]; i < stride; i += incrementTable[component_idx++])
      {
#if LED_STRIPS_SEQUENTIAL
        int color_bit;
        int color_bit_mask = 0x80;

        for (color_bit = 8; color_bit > 0; color_bit--)
          {
            u_int32_t low_gpios = 0;
            int idx;
            int pos = i;
            for (idx = 0; idx < gpio_used; idx++)
              {
                if (pos >= len || !(buff[pos] & color_bit_mask)) low_gpios |= gpio_bit[idx];
                pos += stride;
              }
            led_bits_m_inverted(low_gpios);
            color_bit_mask >>= 1;
          }
#else
        unsigned char c = buff[i];

        if (c & 0x80) led_bits_m_inverted(0); else led_bits_m_inverted(gpio_bit_mask);
        if (c & 0x40) led_bits_m_inverted(0); else led_bits_m_inverted(gpio_bit_mask);
        if (c & 0x20) led_bits_m_inverted(0); else led_bits_m_inverted(gpio_bit_mask);
        if (c & 0x10) led_bits_m_inverted(0); else led_bits_m_inverted(gpio_bit_mask);
        if (c & 0x08) led_bits_m_inverted(0); else led_bits_m_inverted(gpio_bit_mask);
        if (c & 0x04) led_bits_m_inverted(0); else led_bits_m_inverted(gpio_bit_mask);
        if (c & 0x02) led_bits_m_inverted(0); else led_bits_m_inverted(gpio_bit_mask);
        if (c & 0x01) led_bits_m_inverted(0); else led_bits_m_inverted(gpio_bit_mask);
#endif
        if (component_idx >= numComponents) component_idx = 0;
      }
      spin_unlock_irqrestore(&critical, flags);
    }
  else  // not inverted
    {
      // wait 65uS
      for(i=0;i<1000;i++)
      {
        volatile int j = i;
        SET_GPIOS_L(gpio_bit_mask);
      }

      spin_lock_irqsave(&critical, flags);
      for (i = incrementTable[numComponents]; i < stride; i += incrementTable[component_idx++])
      {
#if LED_STRIPS_SEQUENTIAL
        int color_bit;
        int color_bit_mask = 0x80;

        for (color_bit = 8; color_bit > 0; color_bit--)
          {
            u_int32_t low_gpios = 0;
            int idx;
            int pos = i;
            for (idx = 0; idx < gpio_used; idx++)
              {
                if (pos >= len || !(buff[pos] & color_bit_mask)) low_gpios |= gpio_bit[idx];
                pos += stride;
              }
            led_bits_m(low_gpios);
            color_bit_mask >>= 1;
          }
#else
        unsigned char c = buff[i];

        if (c & 0x80) led_bits_m(0); else led_bits_m(gpio_bit_mask);
        if (c & 0x40) led_bits_m(0); else led_bits_m(gpio_bit_mask);
        if (c & 0x20) led_bits_m(0); else led_bits_m(gpio_bit_mask);
        if (c & 0x10) led_bits_m(0); else led_bits_m(gpio_bit_mask);
        if (c & 0x08) led_bits_m(0); else led_bits_m(gpio_bit_mask);
        if (c & 0x04) led_bits_m(0); else led_bits_m(gpio_bit_mask);
        if (c & 0x02) led_bits_m(0); else led_bits_m(gpio_bit_mask);
        if (c & 0x01) led_bits_m(0); else led_bits_m(gpio_bit_mask);
#endif
        if (component_idx >= numComponents) component_idx = 0;
      }
      spin_unlock_irqrestore(&critical, flags);
    }
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
#define DEVICE_NAME "ws2812" /* Dev name as it appears in /proc/ws2812   */

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
static struct class *ws2812_class;

int init_module(void)
{
  int r;
  if ((r = register_chrdev(WS2812_MAJOR, DEVICE_NAME, &fops)))
    {
      printk(KERN_ALERT DEVICE_NAME": Registering char device major=%d failed with %d\n", WS2812_MAJOR, r);
      return r;
    }

  if (gpio_count > GPIO_LIST_MAX)
    {
      printk(KERN_ALERT DEVICE_NAME": Error: gpio_count=%d > MAX=%d\n", gpio_count, GPIO_LIST_MAX);
      return -EINVAL;
    }

  if (gpios && gpio_number >= 0)
    {
      printk(KERN_ALERT DEVICE_NAME": Error: specify either gpios='%s' or gpio_number=%d, not both\n", gpios, gpio_number);
      return -EINVAL;
    }

  gpio_bit_mask = 0;

  if (gpios)
    {
      int idx = 0;
      char *p = gpios;
      while (*p)
        {
          int n = 0;
          while (*p && (*p < '0' || *p > '9')) p++;
          if (!*p) break;
          while (*p >= '0' && *p <= '9') n = 10*n + *p++ - '0';
          gpio_list[idx] = n;
          gpio_bit[idx]  = 1<<n;
          gpio_bit_mask |= 1<<n;
          idx++;
        }
      gpio_number = gpio_list[0];
      gpio_count = idx;
      if (!gpio_count)
        {
          printk(KERN_ALERT DEVICE_NAME": Error: cannot parse pos=%d at list of comma separated integers: gpios='%s'\n", p-gpios, gpios);
          return -EINVAL;
        }
    }
  else
    {
      int idx;
      if (gpio_number < 0) gpio_number = GPIO_NUMBER_DEFAULT;
      // 3 chains: 7<<gpio_number
      for (idx = gpio_number; idx < gpio_number+gpio_count; idx++)
        {
          gpio_list[idx-gpio_number] = idx;
          gpio_bit[idx-gpio_number] = 1<<idx;
          gpio_bit_mask |= 1<<idx;
        }
    }

  // printk(KERN_INFO DEVICE_NAME": Build: %s %s\n", __DATE__, __TIME__); // do not use __DATE__/__TIME__ -> prevents reproducible builds
  printk(KERN_INFO DEVICE_NAME": Major=%d  device=/dev/%s\n", WS2812_MAJOR, DEVICE_NAME);
  if (gpios) printk(KERN_INFO "ws2812: gpios='%s'\n", gpios);
  printk(KERN_INFO DEVICE_NAME": Base GPIO number: %d, gpio_count=%d\n", gpio_number, gpio_count);
  printk(KERN_INFO DEVICE_NAME": Leds per chain: %d\n", leds_per_gpio);
  printk(KERN_INFO DEVICE_NAME": Inverted: %d\n", inverted);
  printk(KERN_INFO DEVICE_NAME": RGBW LEDs: %d\n", rgbw);

  ws2812_class = class_create(THIS_MODULE, DEVICE_NAME);
  if (!device_create(ws2812_class, NULL, MKDEV(WS2812_MAJOR, 0), NULL, DEVICE_NAME ))
    {
      printk(KERN_ALERT DEVICE_NAME": device_create failed. Try 'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, WS2812_MAJOR);
      // return -ENXIO;         // better continue anyway...
    }

  return SUCCESS;
}


/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
  // if we don't survive this, sysfs remains in a broken state.
  // You will see later: sysfs: cannot create duplicate filename '/class/ws2812'
  device_destroy(ws2812_class, MKDEV(WS2812_MAJOR,0));
  class_destroy(ws2812_class);
  unregister_chrdev(WS2812_MAJOR, DEVICE_NAME);
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
  //printk(KERN_INFO DEVICE_NAME": GOT: %s, %d\n", buff, len);
  return len;
}
