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
 *      ledtype=0               # 0=WS281x (GRB), 1=SK6812 (RGBW), 2=P9823 (RGB), default = 0
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

#include <linux/interrupt.h>
#include <linux/irq.h>

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


typedef enum {
  ledtype_ws281x, // GRB bit order
  ledtype_sk6812, // RGBW bit order
  ledtype_p9823 // RGB bit order
} LedType;


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

static int ledtype = ledtype_ws281x;
module_param(ledtype, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(ledtype, "LED type: 0=WS281x, 1=SK6812, 2=P9823");

static int led_count = 1; // default is 1, one single LED
module_param(led_count, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(led_count, "number of LEDs");


static int dma_req = 32; // default -1 => PWM_CHANNEL_DEFAULT
module_param(dma_req, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(pwm_channel, "DMA request line to try");



// timing
#define ACTIVE_BIT_TIME 17 // high time 420nS/840nS - nominally: 350nS/900nS±150nS (25nS/clk@40Mhz)
#define PASSIVE_BIT_TIME 40 // low time is 1000nS - nominally: >900nS/>350nS±150nS (25nS/clk@40Mhz)

#define RELOAD_INTERVAL_NS ((ACTIVE_BIT_TIME*2+PASSIVE_BIT_TIME)*25*(22+1)) // maximum sending time for 64 bits = all H bits = sending 21.333 * 0b110
#define TIMING_EXCEEDED_NS (30*1000+RELOAD_INTERVAL_NS) // if this time is exceeded, we need to restart sending the entire chain because chips might have reset
#define CHAIN_RESET_NS (TIMING_EXCEEDED_NS*2) // x2 safety margin
#define MAX_SEND_REPEATS 3

// spinlock for updating hardware
spinlock_t updatelock;

// DMA output buffer
static size_t dma_buffer_size;
static dma_addr_t dma_buffer;
static void *dma_buffer_vaddr;

#define DMA_TO_PWM 0
#define DMA_CHANNEL 2 // FIXME: UGLY: this is just a channel that seems to be free normally, but maybe is NOT free!

#if !DMA_TO_PWM

// timer controlled PWM feeding
// - total
static u32 *pwmdataptr;
static u32 pwmwordstosend = 0; // initially: none
// - current
static u32 *pwmoutptr;
static u32 pwmwordsleft;
static long long last64BitsSent; // time when last 64bits were put into SENDDATA0/1
static int sendRepeats;

// - HR timer to reload data
struct hrtimer updatetimer;

static void startSendingPWM(void);
static u32 sendNext64bits(void);

static int irqcount = 0;

static enum hrtimer_restart p44ledchain_timer_func(struct hrtimer *timer)
{
  u32 remaining = 0;
  long timerReloadNs = 0;
  unsigned long irqflags;
  int rep;
  long long sinceLastSend;

  spin_lock_irqsave(&updatelock, irqflags);
  // check if this is the chain reset end delay
  if (pwmwordsleft==0) {
    // timer was armed with nothing left to send -> this was the reset timer -> completely done now
    pwmwordstosend = 0;
  }
  else {
    // check if delays might have reset the chain
    if (last64BitsSent)
      sinceLastSend = ktime_to_ns(updatetimer.base->get_time())-last64BitsSent;
    else
      sinceLastSend = 0; // nothing sent yet
    if (sinceLastSend>TIMING_EXCEEDED_NS) {
      // check repeat count
      sendRepeats++;
      if (sendRepeats<MAX_SEND_REPEATS) {
        // - delay restart sending long enough to make sure chain will reset
        timerReloadNs = CHAIN_RESET_NS;
        // - re-initiate sending everything, but not now -> later
        pwmoutptr = pwmdataptr;
        pwmwordsleft = pwmwordstosend;
      }
      else {
        // give up, do not send anything more, let chain reset
        pwmwordsleft = 0;
      }
    }
    else {
      // timing fine, can send now
      remaining = sendNext64bits();
      if (remaining) {
        // more to send
        timerReloadNs = RELOAD_INTERVAL_NS; // normal reload time
      }
      else {
        // nothing remains after this, add a safe delay for chain reset
        timerReloadNs = CHAIN_RESET_NS;
      }
    }
  }
  spin_unlock_irqrestore(&updatelock, irqflags);
  if (!remaining) {
    printk(KERN_INFO DEVICE_NAME": no data remaining, timerReloadNs=%ld nS, sendRepeats=%d, pwmwordsleft=%u, pwmwordstosend=%u\n", timerReloadNs, sendRepeats, pwmwordsleft, pwmwordstosend);
  }
  if (timerReloadNs) {
    // do not formward (because that is relative to the expiry), but start a new timer NOW
    hrtimer_start(&updatetimer, ktime_set(0, timerReloadNs), HRTIMER_MODE_REL);
//     rep = hrtimer_forward_now(&updatetimer, ktime_set(0, timerReloadNs));
//     if (unlikely(rep>1)) {
//       // Note: docs are misleading: hrtimer_forward_now() returns number of intervals added. So 1 is the normal case when retriggering a timer
//       printk(KERN_INFO DEVICE_NAME": timing error, hrtimer_forward returned %d\n", rep);
//     }
//     // still, restart
//     return HRTIMER_RESTART;
  }
  // no more data, do not restart
  return HRTIMER_NORESTART;
}


// IRQs blocked or IRQ context!
void startSendingPWM()
{
  // init the PWM
  sendRepeats = 0;
  // - disable the PWM
  iowrite32(ioread32(PWMENABLE) & ~(1<<pwm_channel), PWMENABLE); // disable PWM
  if (pwmwordstosend>0) {
    // - set up PWM for one output sequence
    iowrite32(0x7E08 | (inverted ? 0x0180 : 0x0000), PWM_CHAN(pwm_channel, PWMCON)); // PWMxCON: New PWM mode, all 64 bits, idle&guard=inverted, 40Mhz clock, no clock dividing
    iowrite32(ACTIVE_BIT_TIME, PWM_CHAN(pwm_channel, inverted ? PWMLDUR : PWMHDUR)); // bit active time
    iowrite32(PASSIVE_BIT_TIME, PWM_CHAN(pwm_channel, inverted ? PWMHDUR : PWMLDUR)); // bit passive time
    iowrite32(0, PWM_CHAN(pwm_channel, PWMGDUR)); // no guard time
    iowrite32(1, PWM_CHAN(pwm_channel, PWMWAVENUM)); // one single wave at a time
    // - initiate sending
    pwmoutptr = pwmdataptr;
    pwmwordsleft = pwmwordstosend;
    // just start a little later
    last64BitsSent = 0;
    hrtimer_start(&updatetimer, ktime_set(0, PASSIVE_BIT_TIME), HRTIMER_MODE_REL);
//     if (sendNext64bits()) {
//       // more data to send
//       hrtimer_start(&updatetimer, ktime_set(0, RELOAD_INTERVAL_NS), HRTIMER_MODE_REL);
//     }
//     else {
//       // all data sent, just time the chain reset
//       printk(KERN_INFO DEVICE_NAME": special case, <64 bit PWM, no reload needed -> just timing chain reset now\n");
//       hrtimer_start(&updatetimer, ktime_set(0, CHAIN_RESET_NS), HRTIMER_MODE_REL);
//     }
  }
}


// IRQs blocked or IRQ context!
u32 sendNext64bits()
{
  if (pwmwordsleft>0) {
    // just send data directly
    iowrite32(ioread32(PWMENABLE) & ~(1<<pwm_channel), PWMENABLE); // disable PWM
    iowrite32(*pwmoutptr, PWM_CHAN(pwm_channel, PWMSENDDATA0)); // Upper 32 bits
    pwmwordsleft--;
    pwmoutptr++;
    if (pwmwordsleft>0) {
      iowrite32(*pwmoutptr, PWM_CHAN(pwm_channel, PWMSENDDATA1)); // Lower 32 bits
      pwmwordsleft--;
      pwmoutptr++;
    }
    else {
      iowrite32(inverted ? 0xFFFFFFFF : 0, PWM_CHAN(pwm_channel, PWMSENDDATA1)); // inactive end
    }
    iowrite32(ioread32(PWMENABLE) | (1<<pwm_channel), PWMENABLE); // (re)enable PWM
    last64BitsSent = ktime_to_ns(updatetimer.base->get_time());
  }
  return pwmwordsleft; // 0 if done, >0 if more to be transferred
}


static irqreturn_t p44ledchain_pwm_interrupt(int irq, void *dev_id)
{
  irqcount++;
  return IRQ_HANDLED;
}



#endif


static void generateBit(int aBit, u32 **aOutbuf, u32 *aOutMask, u32 *aBitcount)
{
  u32 *op = *aOutbuf;
  u32 om = *aOutMask;
  if (om==0) {
    om=1L; // fill LSB first
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
  om = om << 1;
  if (om==0) {
    // begin next longword
    *aOutbuf = op+1;
  }
  *aOutMask = om;
  (*aBitcount)++;
}


// generate bit pattern to be fed into PWM engine:
// one input high bit generates a 110 sequence, one input low bit generates a 100 sequence
static void generateBits(u32 aWord, u8 aNumBits, u32 **aOutbuf, u32 *aOutMask, u32 *aBitcount)
{
  u32 inMask = 1L<<31;
  int bit;
  while (aNumBits>0) {
    // generate next bit
    bit = (aWord & inMask) != 0;
    // make sure 1-bit does not start at end of a 64-bit word
    if (bit && ((*aBitcount&0x3F)==63)) {
      // High bit starting at end of 64bit output word -> would fail because cut in two parts
      generateBit(0, aOutbuf, aOutMask, aBitcount); // insert an extra inactive period, so bit is in fresh 64-bit word
    }
    generateBit(1, aOutbuf, aOutMask, aBitcount); // first bit always high
    if (bit) generateBit(1, aOutbuf, aOutMask, aBitcount); // generate a second high period for a high input bit
//    generateBit(bit, aOutbuf, aOutMask); // middle bit depends on input data bit
    generateBit(0, aOutbuf, aOutMask, aBitcount); // last bit always low
    // shift input bit mask to next bit
    inMask = inMask>>1;
    // bit done
    aNumBits--;
  }
}


#define DATA_DUMP 1

void update_leds(const char *buff, size_t len)
{
  u32 outmask = 0;
  u32 ledword;
  const u8 *inptr = buff;
  u32 *outbuf;
  u32 *outptr;
  unsigned long irqflags;
  u32 longwordsToSend;
  int numComponents;
  int leds;
  u32 bitcount;
  #if DATA_DUMP
  int k;
  int idx;
  #endif

  // debug only
  u32 pwm_wave_num;
  u32 pwm_send_wave_num;
  u32 seg_done_status;


  // check if last send is still in progress
  // FIXME: need better solution than just ignoring data:
  //   e.g. stopping current update and starting over is better
  if (pwmwordstosend!=0) {
    printk(KERN_INFO DEVICE_NAME": still busy sending data -> no update\n");
    return;
  }

  printk(KERN_INFO DEVICE_NAME": received %d input bytes\n", len);
  // number of bytes per LED
  numComponents = ledtype==ledtype_sk6812 ? 4 : 3;
  // number of LEDs
  leds = len/numComponents;
  // limit to max
  if (leds>led_count) leds=led_count;
  printk(KERN_INFO DEVICE_NAME": -> data for %d LEDs with %d bytes each\n", leds, numComponents);
  #if DATA_DUMP
  for (idx=0, k=0; k<leds; k++) {
    if (numComponents==4) {
      printk(KERN_INFO DEVICE_NAME": RGBW LED#%03d : R=%3d, G=%3d, B=%3d, W=%3d\n", k, inptr[idx], inptr[idx+1], inptr[idx+2], inptr[idx+3]);
      idx += 4;
    }
    else {
      printk(KERN_INFO DEVICE_NAME": RGB LED#%03d : R=%3d, G=%3d, B=%3d\n", k, inptr[idx], inptr[idx+1], inptr[idx+2]);
      idx += 3;
    }
  }
  #endif
  // generate data into buffer
  bitcount = 0;
  outbuf = (u32*)dma_buffer_vaddr; // cpu/virt address for the buffer
  outptr = outbuf; // start at beginning
  // generate bits into buffer
  while (leds>0) {
    if (ledtype==ledtype_sk6812) {
      // rgbw -> grbw
      ledword =
        inptr[1]<<24 |
        inptr[0]<<16 |
        inptr[2]<<8 |
        inptr[3];
      generateBits(ledword, 32, &outptr, &outmask, &bitcount);
      inptr += 4;
    }
    else if (ledtype==ledtype_p9823) {
      // rgb -> rgb
      ledword =
        inptr[0]<<24 |
        inptr[1]<<16 |
        inptr[2]<<8;
      generateBits(ledword, 24, &outptr, &outmask, &bitcount);
      inptr += 3;
    }
    else {
      // assume rgb -> grb
      ledword =
        inptr[1]<<24 |
        inptr[0]<<16 |
        inptr[2]<<8;
      generateBits(ledword, 24, &outptr, &outmask, &bitcount);
      inptr += 3;
    }
    // next LED
    leds--;
  }
  longwordsToSend = (u32)(outptr-outbuf);
  if (outmask!=0) longwordsToSend++; // Note: outmask==0 means that the longword at outptr was not yet used
  printk(KERN_INFO DEVICE_NAME": bits generated=%u, longwords to send=%u outmask=0x%08X\n", bitcount, longwordsToSend, outmask);
  #if DATA_DUMP
  for (k=0; k<longwordsToSend; k++) {
    printk(KERN_INFO DEVICE_NAME": longwords #%d : 0x%08X\n", k, outbuf[k]);
  }
  #endif
  // pass buffer to PWM
  spin_lock_irqsave(&updatelock, irqflags);
  // get some info
  pwm_wave_num = ioread32(PWM_CHAN(pwm_channel, PWMWAVENUM));
  pwm_send_wave_num = ioread32(PWM_CHAN(pwm_channel, PWMSENDWAVENUM));
  seg_done_status = ioread32(GDMA_DONE_INTSTS);

  #if DMA_TO_PWM
  // Use DMA to feed PWM engine
  // TODO: find out if that's really possible, does not work yet as-is
  // - disable the PWM
  iowrite32(ioread32(PWMENABLE) & ~(1<<pwm_channel), PWMENABLE); // disable PWM
  // - set up PWM for one output sequence
  iowrite32(0x7E08 | (inverted ? 0x0180 : 0x0000), PWM_CHAN(pwm_channel, PWMCON)); // PWMxCON: New PWM mode, all 64 bits, idle&guard=inverted, 40Mhz clock, no clock dividing
  iowrite32(ACTIVE_BIT_TIME, PWM_CHAN(pwm_channel, inverted ? PWMLDUR : PWMHDUR)); // bit active time
  iowrite32(PASSIVE_BIT_TIME, PWM_CHAN(pwm_channel, inverted ? PWMHDUR : PWMLDUR)); // bit passive time
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
  #else
  // Use timer IRQs to feed PWM
  // - init totals (for retries)
  pwmdataptr = outbuf;
  pwmwordstosend = longwordsToSend;
  startSendingPWM();
  #endif
  spin_unlock_irqrestore(&updatelock, irqflags);
  // FIXME: remove debug code
  printk(KERN_INFO DEVICE_NAME": before sending, we had irqcount=%d, pwm_wave_num=%u, pwm_send_wave_num=%u, seg_done_status=0x%08X\n", irqcount, pwm_wave_num, pwm_send_wave_num, seg_done_status);
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

static int irqno = 26+8; // hardcoded for now, supposedly: PWM (guess from MT7628 datasheet, nowhere else documented)

int init_module(void)
{
  int r;

  // check params
  if (pwm_channel >= NUM_PWMS) {
    printk(KERN_ALERT DEVICE_NAME": Error: pwm_channel=%d > MAX=%d\n", pwm_channel, NUM_PWMS-1);
    r = -EINVAL;
    goto err;
  }
  if (led_count >= P44_LEDCHAIN_MAX_LEDS) {
    printk(KERN_ALERT DEVICE_NAME": Error: led_count=%d > MAX=%d\n", led_count, P44_LEDCHAIN_MAX_LEDS-1);
    r = -EINVAL;
    goto err;
  }

  if ((r = register_chrdev(P44_LEDCHAIN_MAJOR, DEVICE_NAME, &fops))) {
    printk(KERN_ALERT DEVICE_NAME": Registering char device major=%d failed with %d\n", P44_LEDCHAIN_MAJOR, r);
    r = -EINVAL;
    goto err;
  }

  r = request_any_context_irq(
    irqno,
    p44ledchain_pwm_interrupt,
    IRQF_TRIGGER_NONE, // as per hardware
    "pwm-irq",
    &pwmdataptr // FIXME: pass device here once we have a struct (and not only statics)
  );
  if (r!=IRQC_IS_HARDIRQ) {
    printk(KERN_ERR DEVICE_NAME": registering IRQ %d failed (or not hardIRQ) for PWM; err=%d\n", irqno, r);
    r = -EINVAL;
    goto err_unreg_device;
  }

  // printk(KERN_INFO DEVICE_NAME": Build: %s %s\n", __DATE__, __TIME__); // do not use __DATE__/__TIME__ -> prevents reproducible builds
  printk(KERN_INFO DEVICE_NAME": Major=%d  device=/dev/%s\n", P44_LEDCHAIN_MAJOR, DEVICE_NAME);
  printk(KERN_INFO DEVICE_NAME": PWM channel=%d\n", pwm_channel);
  printk(KERN_INFO DEVICE_NAME": Number of LEDs: %d\n", led_count);
  printk(KERN_INFO DEVICE_NAME": Inverted: %d\n", inverted);
  printk(KERN_INFO DEVICE_NAME": LED type: %d\n", ledtype);

  p44ledchain_class = class_create(THIS_MODULE, DEVICE_NAME);
  p44ledchain_dev = device_create(p44ledchain_class, NULL, MKDEV(P44_LEDCHAIN_MAJOR, 0), NULL, DEVICE_NAME);
  if (!p44ledchain_dev) {
    printk(KERN_ALERT DEVICE_NAME": device_create failed. Try 'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, P44_LEDCHAIN_MAJOR);
    r = -ENXIO;
    goto err_class_destroy;
  }

  // init the lock
  spin_lock_init(&updatelock);
  // init the timer
  hrtimer_init(&updatetimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  updatetimer.function = p44ledchain_timer_func;

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
    r = -EINVAL;
    goto err_device_destroy;
  }
  printk(KERN_INFO DEVICE_NAME": DMA buffer, size=%d, virtual address=0x%08X, physical address=0x%08X\n", dma_buffer_size, (u32)dma_buffer_vaddr, (u32)dma_buffer);
  return SUCCESS;
err_device_destroy:
  device_destroy(p44ledchain_class, MKDEV(P44_LEDCHAIN_MAJOR,0));
  iounmap(gdma_base);
  iounmap(pwm_base);
err_class_destroy:
  class_destroy(p44ledchain_class);
err_free_irq:
  free_irq(irqno, &pwmdataptr);
err_unreg_device:
  unregister_chrdev(P44_LEDCHAIN_MAJOR, DEVICE_NAME);
err:
  return r;
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

  // cancel the timer
  pwmwordsleft = 0;
  pwmwordstosend = 0;
  hrtimer_cancel(&updatetimer);

  // free the irq
  free_irq(irqno, &pwmdataptr);

  // remove the device
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
