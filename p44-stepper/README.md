p44-stepper
===========

This is a kernel module for driving stepper motors using GPIO outputs.

(c) 2024 by luz@plan44.ch

## Using p44-stepper

after compiling/installing the p44-ledchain kernel module package, activate the driver as follows:

    insmod p44-stepper stepper<Mno>=<activelow>,<type>,<coilA_gpio>,<coilB_gpio>,<coilC_gpio>,<coilD_gpio>

Where

- **Mno** The stepper motor number to use. Can be 0..3.
- **activelow** can be 0 for active high coil outputs, and 1 for inverted coil outputs.
- **type** is the stepper motor type:

  - **0 = unipolar**: A/B are the coils of the first phase, C/D are the coils of the second phase.
    With steps (4 substeps per cycle, one half-coil always powered in each pair AB, CD)
  - **1 = unipolar_hs**: unipolar with half steps (8 substeps per cycle, intermediate steps with only one coil pair powered)
  - All other values are reserved for other types in future versions of this driver

- **coilX_gpio**: GPIO number for the coil connections A,B,C,D. A/B are the positive/negative subcoils of one phase, B/C positive/negative subcoils of the other phase.

So, the following command will create a driver for an unipolar motor on GPIOs 34...37 with active low outputs:

    insmod p44-stepper stepper0=1,1,34,35,36,37

This will produce a `/sys/class/stepper/stepper0` entry in the sysfs with the following files in it:

- **currentstep**: the current step position. Can be set to a new value to re-adjust or zero the current position. Can be positive or negative withing int32 range. Writing also stops movement and sets targetstep to the same value

- **targetstep**: the step position the motor should reach. Writing a new value starts movement

- **stepinterval**: in microseconds: the step interval -> smaller values, higher speed

- **moving**: 1 when in progress reaching currentstep==targetstep, 0 when stopped. Writing 0 also stops movement.

- **power**: 1 when any coil has power. Writing 0 stops movement and removes power from all coils

