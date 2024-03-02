/*
 *  p44-stepper.c - kernel module for driving stepper motors using GPIO outputs
 *
 *  Copyright (C) 2025 Lukas Zeller <luz@plan44.ch>
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


// MARK: ===== Global Module definitions

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lukas Zeller luz@plan44.ch");
MODULE_DESCRIPTION("Driver for stepper motors connected to GPIO lines");
// Version history
// 1.0 - initial version
MODULE_VERSION("1.0");

#define DEVICE_NAME "stepper"

#define LOGPREFIX DEVICE_NAME ": "

// MARK: ===== Module Parameter definitions

// parameter array indices
#define STEPPER_PARAM_ACTIVELOW 0 // active low
#define STEPPER_PARAM_TYPE 1 // motor/coils type
#define STEPPER_PARAM_COIL_A_GPIO 2 // coil connection A
#define STEPPER_PARAM_COIL_B_GPIO 3 // coil connection B
#define STEPPER_PARAM_COIL_C_GPIO 4 // coil connection C
#define STEPPER_PARAM_COIL_D_GPIO 5 // coil connection D
#define STEPPER_PARAM_REQUIRED_COUNT 6 // required number of params

#define MAX_COIL_GPIOS 4 // max number of GPIOs for coil connections

// parameter array storage
static unsigned int stepper0[STEPPER_PARAM_REQUIRED_COUNT] __initdata;
int stepper0_argc = 0;
static unsigned int stepper1[STEPPER_PARAM_REQUIRED_COUNT] __initdata;
int stepper1_argc = 0;
static unsigned int stepper2[STEPPER_PARAM_REQUIRED_COUNT] __initdata;
int stepper2_argc = 0;
static unsigned int stepper3[STEPPER_PARAM_REQUIRED_COUNT] __initdata;
int stepper3_argc = 0;

#define STEPPER_PARM_DESC " config: <activelow>,<type>,<coilA_gpio>,<coilB_gpio>,<coilC_gpio>,<coilD_gpio>"

// parameter declarations
module_param_array(stepper0, int, &stepper0_argc, 0000);
MODULE_PARM_DESC(stepper0, "stepper0" STEPPER_PARM_DESC);
module_param_array(stepper1, int, &stepper1_argc, 0000);
MODULE_PARM_DESC(stepper1, "stepper1" STEPPER_PARM_DESC);
module_param_array(stepper2, int, &stepper2_argc, 0000);
MODULE_PARM_DESC(stepper2, "stepper2" STEPPER_PARM_DESC);
module_param_array(stepper3, int, &stepper3_argc, 0000);
MODULE_PARM_DESC(stepper3, "stepper3" STEPPER_PARM_DESC);

// === Stepper types and their parameters

typedef struct {
  const char *name; ///< name of the stepper type
  int substeps; ///< number of substeps
  const u8 *coilstates; ///< coil connection states (active high, B0=A, B1=B...) for each substep (substeps entries)
} StepperTypeDescriptor_t;

typedef enum {
  steppertype_unipolar, ///< uniploar
  steppertype_unipolar_hs, ///< uniploar with half steps
  num_steppertypes
} StepperType_t;

// both coil pairs always powered in one direction = full step
#define unipolar4_substeps 4
static const u8 unipolar4_coilstates[unipolar4_substeps] = {
  0b1010, // 1: AB+CD
  0b0110, // 2: /AB+CD
  0b0101, // 3: /AB+/CD
  0b1001 // 4: AB+/CD
};

// in half steps only one coil pair is powered in one or the other direction
#define unipolar8_substeps 8
static const u8 unipolar8_coilstates[unipolar8_substeps] = {
  0b1010, // 1: AB+CD
  0b0010, // 2: CD
  0b0110, // 3: /AB+CD
  0b0100, // 4: /AB
  0b0101, // 5: /AB+/CD
  0b0001, // 6: /CD
  0b1001, // 7: AB+/CD
  0b1000 // 8: AB
};

static const StepperTypeDescriptor_t stepperTypeDescriptors[num_steppertypes] = {
  // unipolar
  { .name = "unipolar", .substeps = unipolar4_substeps, .coilstates = unipolar4_coilstates },
  { .name = "unipolar_hs", .substeps = unipolar8_substeps, .coilstates = unipolar8_coilstates }
};


// MARK: ===== structs


// device variables record
struct p44stepper_dev {
  // configuration
  int minor;
  // - inverted coil signals?
  int activelow;
  // - stepper descriptor
  const StepperTypeDescriptor_t *stepperDesc;
  // - coil connection GPIOs
  int coil_gpio_nos[MAX_COIL_GPIOS];

  // operating parameters
  // - current substep reached
  s32 currentstep;
  // - target substep to reach
  s32 targetstep;
  // - [µs] speed (substep interval)
  u32 stepinterval_us;

  // internals
  // - the cdev
  struct cdev cdev;
  // - the device
  struct device *device;
  // - stepper state
  u8 coilstate;
  int stepinc; // -1, 0 or 1
  // - HR timer to time steps
  struct hrtimer steptimer;
  // - timer/main lock
  spinlock_t lock;
};
typedef struct p44stepper_dev *devPtr_t;


// MARK: ===== static (module global) vars

// the device class
static struct class *p44stepper_class = NULL;

// the device major number
int p44stepper_major;

// the devices
#define MAX_DEVICES 4
static devPtr_t p44stepper_devices[MAX_DEVICES];


// MARK: ===== Motor Controller Implementation

#define COIL_DEBUG 0

static void apply_coilstate(devPtr_t dev, u8 coilstate)
{
  int i;
  dev->coilstate = coilstate;
  #if COIL_DEBUG
  printk(KERN_INFO LOGPREFIX "ABCD: %c%c%c%c\n",
    coilstate & 0x08 ? 'A' : '-',
    coilstate & 0x04 ? 'B' : '-',
    coilstate & 0x02 ? 'C' : '-',
    coilstate & 0x01 ? 'D' : '-'
  );
  #endif
  for (i=0; i<MAX_COIL_GPIOS; i++) {
    int gn = dev->coil_gpio_nos[i];
    if (gn>=0) {
      // apply to this output
      gpio_set_value(gn, ((coilstate & (1<<(MAX_COIL_GPIOS-1)))!=0) != dev->activelow);
    }
    coilstate <<= 1;
  }
}


static void apply_substep(devPtr_t dev)
{
  int sidx;
  u8 newcoils;
  u8 coildiffs;
  u8 intermediate;
  int substeps = dev->stepperDesc->substeps;

  // figure out the current substep
  sidx = dev->currentstep % substeps;
  if (sidx<0) sidx += substeps;
  #if COIL_DEBUG
  printk(KERN_INFO LOGPREFIX "currentstep = %d, substeps=%d, sidx=%d\n", dev->currentstep, substeps, sidx);
  #endif
  // apply the current substep
  newcoils = dev->stepperDesc->coilstates[sidx];
  coildiffs = newcoils ^ dev->coilstate;
  // intermediate state masks out those that will BECOME active
  intermediate = newcoils & ~(coildiffs & newcoils);
  if (intermediate!=newcoils) {
    // we need to apply an intermediate to avoid crossover shorts
    apply_coilstate(dev, intermediate);
  }
  apply_coilstate(dev, newcoils);
}


static void motor_stop(devPtr_t dev)
{
  // immediate stop (but not unpower)
  dev->stepinc = 0;
  dev->targetstep = dev->currentstep; // implicit: target reached
  // cancel the timer
	hrtimer_cancel(&dev->steptimer);
}


static void motor_start(devPtr_t dev)
{
  dev->stepinc = dev->currentstep>dev->targetstep ? -1 : 1;
  apply_substep(dev);
  // now running on timer
  hrtimer_start(&dev->steptimer, (ktime_t)(dev->stepinterval_us * NSEC_PER_USEC), HRTIMER_MODE_REL);
}


static int motor_moving(devPtr_t dev)
{
  return dev->stepinc!=0;
}


static int motor_powered(devPtr_t dev)
{
  return dev->coilstate!=0; // powered when any coil is on
}


static void motor_power(devPtr_t dev, int power)
{
  if (!power) {
    motor_stop(dev);
    apply_coilstate(dev, 0);
  }
  else {
    apply_substep(dev);
  }
}


static void movement_check(devPtr_t dev)
{
  if (dev->currentstep!=dev->targetstep && !motor_moving(dev)) {
    motor_start(dev);
  }
  else if (dev->currentstep==dev->targetstep && motor_moving(dev)) {
    motor_stop(dev);
  }
}


static void motor_set_currentstep(devPtr_t dev, s32 currentstep)
{
  motor_stop(dev);
  dev->currentstep = currentstep;
  dev->targetstep = currentstep;
}


static void motor_set_targetstep(devPtr_t dev, s32 targetstep)
{
  dev->targetstep = targetstep;
  movement_check(dev);
}



static enum hrtimer_restart p44stepper_timer_func(struct hrtimer *timer)
{
  unsigned long irqflags;
  int rep;
  devPtr_t dev = container_of(timer, struct p44stepper_dev, steptimer);
  ktime_t timerReloadNs;

  spin_lock_irqsave(&dev->lock, irqflags);
  if (dev->stepinc!=0) {
    // still moving
    // - do the step
    dev->currentstep += dev->stepinc;
    apply_substep(dev);
    // - done?
    if (dev->currentstep==dev->targetstep) {
      // reached position
      dev->stepinc = 0;
      timerReloadNs = 0;
    }
    else {
      // need another step
      timerReloadNs = (ktime_t)dev->stepinterval_us * NSEC_PER_USEC;
    }
  }
  else {
    // no more timer events needed
    timerReloadNs = 0;
  }
  spin_unlock_irqrestore(&dev->lock, irqflags);
  if (timerReloadNs>0) {
    // re-arm timer
    rep = hrtimer_forward_now(&dev->steptimer, timerReloadNs);
    if (unlikely(rep>1)) {
      // Note: docs are misleading: hrtimer_forward_now() returns number of intervals added. So 1 is the normal case when retriggering a timer
      printk(KERN_INFO LOGPREFIX "timing error, hrtimer_forward returned %d\n", rep);
      return HRTIMER_NORESTART;
    }
    return HRTIMER_RESTART;
  }
  return HRTIMER_NORESTART;
}



// MARK: ===== Attributes

// Useful info for device with sysfs attributes
//   https://gist.github.com/dhilst/5945520


static ssize_t currentstep_show(struct device *device, struct device_attribute *attr, char *buf)
{
  unsigned long irqflags;
  s32 tmp;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  spin_lock_irqsave(&dev->lock, irqflags);
  tmp = dev->currentstep;
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return sysfs_emit(buf, "%d\n", tmp);
}

static ssize_t currentstep_store(struct device *device, struct device_attribute *attr, const char *buf, size_t count)
{
  int tmp;
  unsigned long irqflags;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  sscanf(buf, "%d", &tmp);
  spin_lock_irqsave(&dev->lock, irqflags);
  motor_set_currentstep(dev, tmp);
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return count;
}

static DEVICE_ATTR(currentstep, S_IRUSR | S_IWUSR, currentstep_show, currentstep_store);


static ssize_t targetstep_show(struct device *device, struct device_attribute *attr, char *buf)
{
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  // no lock required, is never changed other than from here
  return sysfs_emit(buf, "%d\n", dev->targetstep);
}

static ssize_t targetstep_store(struct device *device, struct device_attribute *attr, const char *buf, size_t count)
{
  int tmp;
  unsigned long irqflags;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  sscanf(buf, "%d", &tmp);
  spin_lock_irqsave(&dev->lock, irqflags);
  motor_set_targetstep(dev, tmp);
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return count;
}

static DEVICE_ATTR(targetstep, S_IRUSR | S_IWUSR, targetstep_show, targetstep_store);


static ssize_t stepinterval_show(struct device *device, struct device_attribute *attr, char *buf)
{
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  // no lock required, is never changed other than from here
  return sysfs_emit(buf, "%u\n", dev->stepinterval_us);
}

static ssize_t stepinterval_store(struct device *device, struct device_attribute *attr, const char *buf, size_t count)
{
  unsigned int tmp;
  unsigned long irqflags;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  sscanf(buf, "%u", &tmp);
  spin_lock_irqsave(&dev->lock, irqflags);
  dev->stepinterval_us = tmp;
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return count;
}

static DEVICE_ATTR(stepinterval, S_IRUSR | S_IWUSR, stepinterval_show, stepinterval_store);


static ssize_t power_show(struct device *device, struct device_attribute *attr, char *buf)
{
  unsigned long irqflags;
  int tmp;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  spin_lock_irqsave(&dev->lock, irqflags);
  tmp = motor_powered(dev);
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return sysfs_emit(buf, "%u\n", tmp);
}

static ssize_t power_store(struct device *device, struct device_attribute *attr, const char *buf, size_t count)
{
  unsigned long irqflags;
  int tmp;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  sscanf(buf, "%d", &tmp);
  spin_lock_irqsave(&dev->lock, irqflags);
  motor_power(dev, tmp);
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return count;
}

static DEVICE_ATTR(power, S_IRUSR | S_IWUSR, power_show, power_store);


static ssize_t moving_show(struct device *device, struct device_attribute *attr, char *buf)
{
  unsigned long irqflags;
  int tmp;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  spin_lock_irqsave(&dev->lock, irqflags);
  tmp = motor_moving(dev);
  spin_unlock_irqrestore(&dev->lock, irqflags);
  return sysfs_emit(buf, "%u\n", tmp);
}

static ssize_t moving_store(struct device *device, struct device_attribute *attr, const char *buf, size_t count)
{
  unsigned long irqflags;
  int tmp;
  devPtr_t dev = (devPtr_t)dev_get_drvdata(device);
  sscanf(buf, "%d", &tmp);
  if (!tmp) {
    spin_lock_irqsave(&dev->lock, irqflags);
    motor_stop(dev);
    spin_unlock_irqrestore(&dev->lock, irqflags);
  }
  return count;
}

static DEVICE_ATTR(moving, S_IRUSR | S_IWUSR, moving_show, moving_store);



// MARK: ===== character device file operations

// prototypes
static int p44stepper_open(struct inode *, struct file *);
static int p44stepper_release(struct inode *, struct file *);
static ssize_t p44stepper_read(struct file *, char *, size_t, loff_t *);
static ssize_t p44stepper_write(struct file *, const char *, size_t, loff_t *);

// file access handlers
static struct file_operations p44stepper_fops = {
  .open = p44stepper_open,
  .release = p44stepper_release,
  .read = p44stepper_read,
  .write = p44stepper_write,
};


static int p44stepper_open(struct inode *inode, struct file *filp)
{
  // NOP;
  return 0;
}


static int p44stepper_release(struct inode *inode, struct file *filp)
{
  // NOP
  return 0;
}


static ssize_t p44stepper_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
  // NOP
  return 0;
}


static ssize_t p44stepper_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
  // silently discard
  return len;
}



// MARK: ===== device init and cleanup

static int p44stepper_add_device(struct class *class, int minor, devPtr_t *devP, unsigned int *params, int param_count, const char *devname)
{
  int err;
  int pval;
	devPtr_t dev = NULL;
	int i;
  char nm[5+1]; // "coilX"

	BUG_ON(class==NULL || devP==NULL);

  // no dev created yet
  *devP = NULL;
  // check param count
  if (param_count<STEPPER_PARAM_REQUIRED_COUNT) {
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
  // - active low
  dev->activelow = params[STEPPER_PARAM_ACTIVELOW]!=0;
  // - type
  pval = params[STEPPER_PARAM_TYPE];
  if (pval<0 || pval>=num_steppertypes) {
    printk(KERN_WARNING LOGPREFIX "Invalid type, must be 0..%d\n", num_steppertypes-1);
    err = -EINVAL;
    goto err_free;
  }
  dev->stepperDesc = &stepperTypeDescriptors[pval];
  // - coil GPIOs
  dev->coil_gpio_nos[0] = params[STEPPER_PARAM_COIL_A_GPIO];
  dev->coil_gpio_nos[1] = params[STEPPER_PARAM_COIL_B_GPIO];
  dev->coil_gpio_nos[2] = params[STEPPER_PARAM_COIL_C_GPIO];
  dev->coil_gpio_nos[3] = params[STEPPER_PARAM_COIL_D_GPIO];
  // - request and init the GPIOs
  strcpy(nm,"coil");
  nm[5] = 0;
  for (i = 0; i<MAX_COIL_GPIOS; i++) {
    nm[4] = 'A'+i;
    err = gpio_request(dev->coil_gpio_nos[i], nm);
    if (err) {
      printk(KERN_ERR LOGPREFIX "gpio_request failed for %s, gpio %d; err=%d\n", nm, dev->coil_gpio_nos[i], err);
      goto err_free_gpios;
    }
    if (gpio_cansleep(dev->coil_gpio_nos[i])) {
      printk(KERN_ERR LOGPREFIX "gpio %d for %s can sleep -> error, driver needs non-sleeping GPIOs\n", dev->coil_gpio_nos[i], nm);
      goto err_free_gpios;
    }
    // enable output with inactive level to begin with
    err = gpio_direction_output(dev->coil_gpio_nos[i], dev->activelow);
    if (err) {
      printk(KERN_ERR LOGPREFIX "gpio_direction_output failed for %s, gpio %d; err=%d\n", nm, dev->coil_gpio_nos[i], err);
      goto err_free_gpios;
    }
  }
  // init operational params
  dev->currentstep = 0;
  dev->targetstep = 0;
  dev->stepinterval_us = 20*USEC_PER_MSEC;
  dev->coilstate = 0; // all coil outputs inactive
  dev->stepinc = 0; // not moving
  // register cdev
  // - init the struct contained in our dev struct
  cdev_init(&dev->cdev, &p44stepper_fops);
  dev->cdev.owner = THIS_MODULE;
  // - add one device starting at devno (major+minor)
  err = cdev_add(&dev->cdev, MKDEV(p44stepper_major, minor), 1);
  if (err) {
    printk(KERN_WARNING LOGPREFIX "Error adding cdev, err=%d\n", err);
    goto err_free_gpios;
  }
  // create device
  dev->device = device_create(
    class, NULL, // no parent device
		MKDEV(p44stepper_major, minor),
		dev, // pass our dev as drvdata
		devname // device name (format string + more params are allowed)
	);
	if (IS_ERR(dev->device)) {
		err = PTR_ERR(dev->device);
		printk(KERN_WARNING LOGPREFIX "Error %d while trying to create %s\n", err, devname);
		goto err_free_cdev;
	}
	// create the attributes in the /sys/class/stepper/stepperX directory
  err = device_create_file(dev->device, &dev_attr_currentstep);
  if (err<0) goto err_free_device;
  err = device_create_file(dev->device, &dev_attr_targetstep);
  if (err<0) goto err_free_device;
  err = device_create_file(dev->device, &dev_attr_stepinterval);
  if (err<0) goto err_free_device;
  err = device_create_file(dev->device, &dev_attr_power);
  if (err<0) goto err_free_device;
  err = device_create_file(dev->device, &dev_attr_moving);
  if (err<0) goto err_free_device;
  // init the lock
  spin_lock_init(&dev->lock);
  // init the timer
  hrtimer_init(&dev->steptimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  dev->steptimer.function = p44stepper_timer_func;
  // Config summary
  printk(KERN_INFO LOGPREFIX "Device: /dev/%s\n", devname);
  printk(KERN_INFO LOGPREFIX "- Motor type     : %s\n", dev->stepperDesc->name);
  printk(KERN_INFO LOGPREFIX "- Output GPIOs   : active %s: A=%d, B=%d, C=%d, D=%d\n", dev->activelow ? "LOW" : "HIGH", dev->coil_gpio_nos[0], dev->coil_gpio_nos[1], dev->coil_gpio_nos[2], dev->coil_gpio_nos[3]);
  // done
  *devP = dev; // pass back new dev
  return 0;
// wind-down after error
err_free_device:
  device_destroy(class, MKDEV(p44stepper_major, minor));
err_free_cdev:
  cdev_del(&dev->cdev);
err_free_gpios:
  for (i = 0; i<MAX_COIL_GPIOS; i++) {
    // is safe to call for numbers we haven't successfully claimed before
    gpio_free(dev->coil_gpio_nos[i]);
  }
err_free:
  kfree(dev);
err:
  return err;
}


static void p44stepper_remove_device(struct class *class, int minor, devPtr_t *devP)
{
  devPtr_t dev;
  int i;

	BUG_ON(class==NULL || devP==NULL);
  dev = *devP;
  if (!dev) return; // no device to remove
	// immediate stop (and cancel timer)
	motor_stop(dev);
	// destroy device
	device_destroy(class, MKDEV(p44stepper_major, minor));
	// delete cdev
	cdev_del(&dev->cdev);
  // free the GPIOs
  for (i = 0; i<MAX_COIL_GPIOS; i++) {
    gpio_free(dev->coil_gpio_nos[i]);
  }
  // delete dev
  kfree(dev);
  *devP = NULL;
	return;
}


// MARK: ===== module init and exit


static int __init p44stepper_init_module(void)
{
  int err;
  int i;
  dev_t devno;

  // no devices to begin with
  for (i=0; i<MAX_DEVICES; i++) {
    p44stepper_devices[i] = NULL;
  }
  // at least one device needs to be defined
  if (stepper0_argc+stepper1_argc+stepper2_argc+stepper3_argc==0) {
    printk(KERN_WARNING LOGPREFIX "must specify at least one stepper motor\n");
		err = -EINVAL;
		goto err;
  }
	// Get a range of minor numbers (starting with 0) to work with */
	err = alloc_chrdev_region(&devno, 0, MAX_DEVICES, DEVICE_NAME);
	if (err < 0) {
		printk(KERN_WARNING LOGPREFIX "alloc_chrdev_region() failed\n");
		return err;
	}
	p44stepper_major = MAJOR(devno);
	// Create device class
	p44stepper_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(p44stepper_class)) {
		err = PTR_ERR(p44stepper_class);
		goto err_unregister_region;
	}
  // instantiate devices from module params
  if (stepper0_argc>0) {
    err = p44stepper_add_device(p44stepper_class, 0, &(p44stepper_devices[0]), stepper0, stepper0_argc, "stepper0");
    if (err) goto err_destroy_devices;
  }
  if (stepper1_argc>0) {
    err = p44stepper_add_device(p44stepper_class, 1, &(p44stepper_devices[1]), stepper1, stepper1_argc, "stepper1");
    if (err) goto err_destroy_devices;
  }
  if (stepper2_argc>0) {
    err = p44stepper_add_device(p44stepper_class, 2, &(p44stepper_devices[2]), stepper2, stepper2_argc, "stepper2");
    if (err) goto err_destroy_devices;
  }
  if (stepper3_argc>0) {
    err = p44stepper_add_device(p44stepper_class, 3, &(p44stepper_devices[3]), stepper3, stepper3_argc, "stepper3");
    if (err) goto err_destroy_devices;
  }
  // done
  return 0;
err_destroy_devices:
  for (i=0; i<MAX_DEVICES; i++) {
    p44stepper_remove_device(p44stepper_class, i, &(p44stepper_devices[i]));
  }
  class_destroy(p44stepper_class);
err_unregister_region:
  unregister_chrdev_region(MKDEV(p44stepper_major, 0), MAX_DEVICES);
err:
  return err;
}



static void __exit p44stepper_exit_module(void)
{
  int i;

  // destroy the devices
  for (i=0; i<MAX_DEVICES; i++) {
    p44stepper_remove_device(p44stepper_class, i, &(p44stepper_devices[i]));
  }
  // destroy the class
  class_destroy(p44stepper_class);
  // done
  printk(KERN_INFO LOGPREFIX "cleaned up\n");
	return;
}

module_init(p44stepper_init_module);
module_exit(p44stepper_exit_module);



