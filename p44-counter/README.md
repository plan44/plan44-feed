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
- **mode:** edge detection mode.
  - **Bit0**: detect rising edges on `gpioA`
  - **Bit1**: detect falling edges on `gpioA`
  - **Bit2**: detect rising edges on `gpioB`
  - **Bit3**: detect falling edges on `gpioB`
  - **Bit4..31**: reserved, must be 0
- **debounce_us**: debouncing time - if >0, edges detected within `debounce_us`
  microseconds after a detected edge will be ignored
- **gpioA**: GPIO number (edges enabled with `mode` count up by default, can be set by `countmode` in sysfs)
- **gpioB**: (optional) second GPIO number (edges enabled with `mode` count down by default, can be set by `countmode` in sysfs)

So, the following command will create a driver counting up for every positive edge
found on GPIO34 with a debouncing time of 3mS (3000µS):

    insmod p44-counter counter0=1,3000,34

This will produce a `/sys/class/counter/counter0` entry in the sysfs with the following files in it:

- **count**: the current counter position. Can be set to a new value to re-adjust or zero the current count. Can be positive or negative withing int32 range.

- **debounce**: the debounce time to use in µS. This is initially the same as the module param **debounce_us**, but can be adjusted during operation when needed.

- **countmode**: (default is 0x02 = A counts up, B counts down)
  - **Bit0**: if set, edges detected on `gpioA` count down, else up.
    In quadrature mode: A input inverter
  - **Bit1**: if set, edges detected on `gpioB` count down, else up.
    In quadrature mode: B input inverter
  - **Bit2**: if set, A/B are considered quadrature encoder signals.
    Otherwise A/B are considered simple edge counting inputs counted according to bit 0/1.
    (Note: quadrature encoder mode needs detection of all edges, so *mode* parameter at module
    instantiation time must have all four lower bits set).


