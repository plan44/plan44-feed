p44-counter
===========

This is a kernel module for counting pulses on GPIO inputs.

Over time, this will get obsolete through more capable upstream drivers based on
the linux kernel generic counter interface. At the time of writing (and needing)
this simple counter however, the target platform had Kernel 5.10 and counter.h
was existing, but not yet established well in terms of available drivers.

(c) 2024 by luz@plan44.ch

## Using p44-counter

after compiling/installing the p44-ledchain kernel module package, activate the driver as follows:

    insmod p44-counter counter<Cno>=<mode>,<debounce_us>,<gpioA>[,<gpioB>]

Where

- **Cno:** The counter number to use. Can be 0..3.
- **mode:** counter mode.
  - **Bit0**: count up rising edges on `gpioA`
  - **Bit1**: count up falling edges on `gpioA`
  - **Bit2**: count down rising edges on `gpioB`
  - **Bit3**: count down falling edges on `gpioB`
  - **Bit4..31**: reserved, must be 0
- **debounce_us**: debouncing time - if >0, edges detected within `debounce_us`
  microseconds after a detected edge will be ignored
- **gpioA**: GPIO number to count up edges on (depending on `mode`)
- **gpioB**: (optional) GPIO number to count down edges on (depending on `mode`)

So, the following command will create a driver counting up for every positive edge
found on GPIO34 with a debouncing time of 3mS (3000µS):

    insmod p44-counter counter0=1,3000,34

This will produce a `/sys/class/counter/counter0` entry in the sysfs with the following files in it:

- **count**: the current counter position. Can be set to a new value to re-adjust or zero the current count. Can be positive or negative withing int32 range.

- **debounce**: the debounce time to use in µS. This is initially the same as the module param **debounce_us**, but can be adjusted during operation when needed.
