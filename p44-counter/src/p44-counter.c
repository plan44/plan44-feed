/*
 *  p44-counter.c - kernel module counting edges on GPIO inputs
 *
 *  Copyright (C) 2024 Lukas Zeller <luz@plan44.ch>
 *
 *  This is free software, licensed under the GNU General Public License v2.
 *  See /LICENSE for more information.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> // printk()
#include <linux/slab.h> // kzalloc()
#include <linux/uaccess.h> // copy_to_user()
#include <linux/moduleparam.h>
#include <linux/stat.h>

#include <linux/types.h>
#include <linux/string.h>

#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/watchdog.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <linux/fs.h>

#include <linux/gpio.h>
#include <linux/hrtimer.h> // high resolution timers
#include <linux/interrupt.h>
#include <linux/irq.h>


// MARK: ===== Global Module definitions

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lukas Zeller luz@plan44.ch");
MODULE_DESCRIPTION("Driver for counting edges on GPIO lines");
// Version history
// 1.0 - initial version
MODULE_VERSION("1.0");

#define DEVICE_NAME "counter"

#define LOGPREFIX DEVICE_NAME ": "

// MARK: ===== Module Parameter definitions

// parameter array indices
#define COUNTER_PARAM_MODE 0 // counting mode/flags
#define COUNTER_PARAM_DEBOUNCE_US 1 // debounce time in µS
#define COUNTER_PARAM_GPIO_A 2 // counting input A
#define COUNTER_PARAM_REQUIRED_COUNT 3 // required number of params
#define COUNTER_PARAM_GPIO_B 3 // counting input B
#define COUNTER_PARAM_MAX_COUNT 4 // max number of params


// parameter array storage
static unsigned int counter0[COUNTER_PARAM_MAX_COUNT] __initdata;
int counter0_argc = 0;
static unsigned int counter1[COUNTER_PARAM_MAX_COUNT] __initdata;
int counter1_argc = 0;
static unsigned int counter2[COUNTER_PARAM_MAX_COUNT] __initdata;
int counter2_argc = 0;
static unsigned int counter3[COUNTER_PARAM_MAX_COUNT] __initdata;
int counter3_argc = 0;

#define COUNTER_PARM_DESC " config: <mode>,<default_debounce_us>,<gpioA>[,<gpioB>]"

// parameter declarations
module_param_array(counter0, int, &counter0_argc, 0000);
MODULE_PARM_DESC(counter0, "counter0" COUNTER_PARM_DESC);
module_param_array(counter1, int, &counter1_argc, 0000);
MODULE_PARM_DESC(counter1, "counter1" COUNTER_PARM_DESC);
module_param_array(counter2, int, &counter2_argc, 0000);
MODULE_PARM_DESC(counter2, "counter2" COUNTER_PARM_DESC);
module_param_array(counter3, int, &counter3_argc, 0000);
MODULE_PARM_DESC(counter3, "counter3" COUNTER_PARM_DESC);


// MARK: ===== structs

// Mode flags
// - Bit 0/1: egde flags for gpioA and counting up
// - Bit 2/3: egde flags for gpioB and counting down
enum {
  pos_edge = 0x01,
  neg_edge = 0x02,
};


// device variables record
struct p44counter_dev {
  // configuration
  int minor;
  // - mode flags
  u32 mode;
  // - GPIOs A and B
  int gpios[2];
  // - [ns] debounce time
  u32 debounce_ns;

  // operating parameters
  // - current substep reached
  s32 counter;

  // internals
  // - the cdev
  struct cdev cdev;
  // - the device
  struct device *device;
  // - time of last edge detected on Gpio A or B
  u64 lastEdge[2];
  // - timer/main lock
  spinlock_t lock;
};
typedef struct p44counter_dev *devPtr_t;


// MARK: ===== static (module global) vars

// the device class
static struct class *p44counter_class = NULL;

// the device major number
int p44counter_major;

// the devices
#define MAX_DEVICES 4
static devPtr_t p44counter_devices[MAX_DEVICES];


// MARK: ===== Counter Implementation

static irqreturn_t p44counter_edge_irq(int irq, void *dev_id, int gpioidx)
{
  devPtr_t dev = dev_id;
  unsigned long irqflags;
  u64 thisEdge;

  spin_lock_irqsave(&dev->lock, irqflags);
  // check debouncing dead time
  thisEdge = ktime_get_ns();
  if (dev->debounce_ns>0) {
    if (dev->lastEdge[gpioidx]>0 && dev->lastEdge[gpioidx]+dev->debounce_ns>thisEdge) {
      // still in debouncing holdoff time -> ignore the edge
      spin_unlock_irqrestore(&dev->lock, irqflags);
      return IRQ_HANDLED;
    }
  }
  // valid edge
  dev->lastEdge[gpioidx] = thisEdge;
  if (gpioidx==0) {
    // gpio A counts up
    dev->counter++;
  }
  else {
    // gpio B counts down
    dev->counter--;
  }
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return IRQ_HANDLED;
}


static irqreturn_t p44counter_edge_A_irq(int irq, void *dev_id)
{
  return p44counter_edge_irq(irq, dev_id, 0);
}


static irqreturn_t p44counter_edge_B_irq(int irq, void *dev_id)
{
  return p44counter_edge_irq(irq, dev_id, 1);
}


// MARK: ===== Attributes

// Useful info for device with sysfs attributes
//   https://gist.github.com/dhilst/5945520


static ssize_t counter_show(struct device *device, struct device_attribute *attr, char *buf)
{
  unsigned long irqflags;
  s32 tmp;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  spin_lock_irqsave(&dev->lock, irqflags);
  tmp = dev->counter;
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return sysfs_emit(buf, "%d\n", tmp);
}

static ssize_t counter_store(struct device *device, struct device_attribute *attr, const char *buf, size_t count)
{
  int tmp;
  unsigned long irqflags;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  sscanf(buf, "%d", &tmp);
  spin_lock_irqsave(&dev->lock, irqflags);
  dev->counter = tmp;
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return count;
}

static DEVICE_ATTR(counter, S_IRUSR | S_IWUSR, counter_show, counter_store);


static ssize_t debounce_show(struct device *device, struct device_attribute *attr, char *buf)
{
  unsigned long irqflags;
  s32 tmp;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  spin_lock_irqsave(&dev->lock, irqflags);
  tmp = dev->debounce_ns/1000;
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return sysfs_emit(buf, "%d\n", tmp);
}

static ssize_t debounce_store(struct device *device, struct device_attribute *attr, const char *buf, size_t count)
{
  int tmp;
  unsigned long irqflags;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  sscanf(buf, "%d", &tmp);
  spin_lock_irqsave(&dev->lock, irqflags);
  dev->debounce_ns = tmp*1000;
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return count;
}

static DEVICE_ATTR(debounce, S_IRUSR | S_IWUSR, debounce_show, debounce_store);



// MARK: ===== character device file operations

// prototypes
static int p44counter_open(struct inode *, struct file *);
static int p44counter_release(struct inode *, struct file *);
static ssize_t p44counter_read(struct file *, char *, size_t, loff_t *);
static ssize_t p44counter_write(struct file *, const char *, size_t, loff_t *);

// file access handlers
static struct file_operations p44counter_fops = {
  .open = p44counter_open,
  .release = p44counter_release,
  .read = p44counter_read,
  .write = p44counter_write,
};


static int p44counter_open(struct inode *inode, struct file *filp)
{
  // NOP;
  return 0;
}


static int p44counter_release(struct inode *inode, struct file *filp)
{
  // NOP
  return 0;
}


static ssize_t p44counter_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
  // NOP
  return 0;
}


static ssize_t p44counter_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
  // silently discard
  return len;
}



// MARK: ===== device init and cleanup

static int p44counter_add_device(struct class *class, int minor, devPtr_t *devP, unsigned int *params, int param_count, const char *devname)
{
  int err;
  int irqno;
	devPtr_t dev = NULL;
	int i;
  char nm[1+1]; // "A"
  u8 edgeflags;
  u32 ef;

	BUG_ON(class==NULL || devP==NULL);

  // no dev created yet
  *devP = NULL;
  // check param count
  if (param_count<COUNTER_PARAM_REQUIRED_COUNT) {
    printk(KERN_WARNING LOGPREFIX "not enough parameters for %s\n", devname);
    err = -EINVAL;
    goto err;
  }
  // create device variables struct
  dev = kzalloc(sizeof(*dev), GFP_KERNEL);
  if (!dev) {
    err = -ENOMEM;
    goto err;
  }
  // assign PWM channel no = minor devno
  dev->minor = minor;
  // parse the params
  // - mode
  dev->mode = (u32)params[COUNTER_PARAM_MODE];
  // - debounce time (specified in uS, stored in nS)
  dev->debounce_ns = (u32)params[COUNTER_PARAM_DEBOUNCE_US]*1000;
  // - GPIOs
  dev->gpios[0] = params[COUNTER_PARAM_GPIO_A];
  dev->gpios[1] = -1; // default to none
  if (param_count>COUNTER_PARAM_GPIO_B) {
    dev->gpios[1] = params[COUNTER_PARAM_GPIO_B];
  }
  // - request and init the GPIOs
  nm[1] = 0;
  for (i = 0; i<2; i++) {
    dev->lastEdge[i]=0;
    nm[0] = 'A'+i;
    if (dev->gpios[i]<0) continue;
    err = gpio_request(dev->gpios[i], nm);
    if (err) {
      printk(KERN_ERR LOGPREFIX "gpio_request failed for %s, gpio %d; err=%d\n", nm, dev->gpios[i], err);
      goto err_free_gpios;
    }
    if (gpio_cansleep(dev->gpios[i])) {
      printk(KERN_ERR LOGPREFIX "gpio %d for %s can sleep -> error, driver needs non-sleeping GPIOs\n", dev->gpios[i], nm);
      goto err_free_gpios;
    }
    err = gpio_direction_input(dev->gpios[i]);
    if (err) {
      printk(KERN_ERR LOGPREFIX "gpio_direction_input failed for %s, gpio %d; err=%d\n", nm, dev->gpios[i], err);
      goto err_free_gpios;
    }
    // request IRQ for GPIO
    irqno = gpio_to_irq(dev->gpios[i]);
    if (irqno<0) {
      printk(KERN_ERR LOGPREFIX "gpio %d does not have an IRQ line assigned; err=%d\n", dev->gpios[i], irqno);
      goto err_free_gpios;
    }
    // which edges?
    edgeflags = ((dev->mode>>(2*i)) & 0x03);
    ef = 0;
    if (edgeflags&pos_edge) ef |= IRQF_TRIGGER_RISING;
    if (edgeflags&neg_edge) ef |= IRQF_TRIGGER_FALLING;
    err = request_any_context_irq(
      irqno,
      i==0 ? p44counter_edge_A_irq : p44counter_edge_B_irq,
      ef,
      "counter-irq",
      dev
    );
    if (err!=IRQC_IS_HARDIRQ) {
      printk(KERN_ERR LOGPREFIX "registering IRQ %d failed (or not hardIRQ) for gpio %d; err=%d\n", irqno, dev->gpios[i], err);
      goto err_free_gpios;
    }
  }
  // init operational params
  dev->counter = 0;
  // register cdev
  // - init the struct contained in our dev struct
  cdev_init(&dev->cdev, &p44counter_fops);
  dev->cdev.owner = THIS_MODULE;
  // - add one device starting at devno (major+minor)
  err = cdev_add(&dev->cdev, MKDEV(p44counter_major, minor), 1);
  if (err) {
    printk(KERN_WARNING LOGPREFIX "Error adding cdev, err=%d\n", err);
    goto err_free_gpios;
  }
  // create device
  dev->device = device_create(
    class, NULL, // no parent device
		MKDEV(p44counter_major, minor),
		dev, // pass our dev as drvdata
		devname // device name (format string + more params are allowed)
	);
	if (IS_ERR(dev->device)) {
		err = PTR_ERR(dev->device);
		printk(KERN_WARNING LOGPREFIX "Error %d while trying to create %s\n", err, devname);
		goto err_free_cdev;
	}
	// create the attributes in the /sys/class/counter/counterX directory
  err = device_create_file(dev->device, &dev_attr_counter);
  err = device_create_file(dev->device, &dev_attr_debounce);
  if (err<0) goto err_free_device;
  // init the lock
  spin_lock_init(&dev->lock);
  // Config summary
  printk(KERN_INFO LOGPREFIX "Device: /dev/%s\n", devname);
  printk(KERN_INFO LOGPREFIX "- Mode     : 0x%u\n", dev->mode);
  printk(KERN_INFO LOGPREFIX "- Debounce : %u uS\n", dev->debounce_ns/1000);
  printk(KERN_INFO LOGPREFIX "- Inputs   : A=%d, B=%d\n", dev->gpios[0], dev->gpios[1]);
  // done
  *devP = dev; // pass back new dev
  return 0;
// wind-down after error
err_free_device:
  device_destroy(class, MKDEV(p44counter_major, minor));
err_free_cdev:
  cdev_del(&dev->cdev);
err_free_gpios:
  for (i = 0; i<2; i++) {
    // is safe to call for numbers we haven't successfully claimed before
    if (dev->gpios[i]<0) continue;
    gpio_free(dev->gpios[i]);
  }
err_free:
  kfree(dev);
err:
  return err;
}


static void p44counter_remove_device(struct class *class, int minor, devPtr_t *devP)
{
  devPtr_t dev;
  int i;

	BUG_ON(class==NULL || devP==NULL);
  dev = *devP;
  if (!dev) return; // no device to remove
	// destroy device
	device_destroy(class, MKDEV(p44counter_major, minor));
	// delete cdev
	cdev_del(&dev->cdev);
  // free the GPIOs
  for (i = 0; i<2; i++) {
    if (dev->gpios[i]<0) continue;
    // free the irq
    free_irq(gpio_to_irq(dev->gpios[i]), dev);
    // free the gpio itself
    gpio_free(dev->gpios[i]);
  }
  // delete dev
  kfree(dev);
  *devP = NULL;
	return;
}


// MARK: ===== module init and exit


static int __init p44counter_init_module(void)
{
  int err;
  int i;
  dev_t devno;

  // no devices to begin with
  for (i=0; i<MAX_DEVICES; i++) {
    p44counter_devices[i] = NULL;
  }
  // at least one device needs to be defined
  if (counter0_argc+counter1_argc+counter2_argc+counter3_argc==0) {
    printk(KERN_WARNING LOGPREFIX "must specify at least one counter\n");
		err = -EINVAL;
		goto err;
  }
	// Get a range of minor numbers (starting with 0) to work with */
	err = alloc_chrdev_region(&devno, 0, MAX_DEVICES, DEVICE_NAME);
	if (err < 0) {
		printk(KERN_WARNING LOGPREFIX "alloc_chrdev_region() failed\n");
		return err;
	}
	p44counter_major = MAJOR(devno);
	// Create device class
	p44counter_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(p44counter_class)) {
		err = PTR_ERR(p44counter_class);
		goto err_unregister_region;
	}
  // instantiate devices from module params
  if (counter0_argc>0) {
    err = p44counter_add_device(p44counter_class, 0, &(p44counter_devices[0]), counter0, counter0_argc, "counter0");
    if (err) goto err_destroy_devices;
  }
  if (counter1_argc>0) {
    err = p44counter_add_device(p44counter_class, 1, &(p44counter_devices[1]), counter1, counter1_argc, "counter1");
    if (err) goto err_destroy_devices;
  }
  if (counter2_argc>0) {
    err = p44counter_add_device(p44counter_class, 2, &(p44counter_devices[2]), counter2, counter2_argc, "counter2");
    if (err) goto err_destroy_devices;
  }
  if (counter3_argc>0) {
    err = p44counter_add_device(p44counter_class, 3, &(p44counter_devices[3]), counter3, counter3_argc, "counter3");
    if (err) goto err_destroy_devices;
  }
  // done
  return 0;
err_destroy_devices:
  for (i=0; i<MAX_DEVICES; i++) {
    p44counter_remove_device(p44counter_class, i, &(p44counter_devices[i]));
  }
  class_destroy(p44counter_class);
err_unregister_region:
  unregister_chrdev_region(MKDEV(p44counter_major, 0), MAX_DEVICES);
err:
  return err;
}



static void __exit p44counter_exit_module(void)
{
  int i;

  // destroy the devices
  for (i=0; i<MAX_DEVICES; i++) {
    p44counter_remove_device(p44counter_class, i, &(p44counter_devices[i]));
  }
  // destroy the class
  class_destroy(p44counter_class);
  // done
  printk(KERN_INFO LOGPREFIX "cleaned up\n");
	return;
}

module_init(p44counter_init_module);
module_exit(p44counter_exit_module);



