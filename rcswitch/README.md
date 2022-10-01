**Note by luz@plan44.ch, March 2018, Sept 2022:** this OpenWrt package is a kernel-module-only
extract of Stefan Wendler's original project `rcswitch-kmod`, which can be found
on [github.com/wendlers/rcswitch-kmod](https://github.com/wendlers/rcswitch-kmod).

Full credits to Stefan Wendler <sw@kaltpost.de> for this kernel module!

This README is also an extract of the original, containing the relevant parts for
operating the kernel module via sysfs, but leaves out information which only applies
to the original project.

RCSwitch Linux Kernel module - operates 434MHz RCSwitches
=========================================================
18.07.2014 Stefan Wendler
sw@kaltpost.de

Kernel module to operate RC power outlets (switches) through SYSFS. The module
was devoped for a MIPS based Carambola board, but should run on any other Linux
board as well.

The module exports a command interface to the sysfs which allows sending on/off
commands to the RC swtiches. For example:


	# turn switch off
	echo "11111A0" > /sys/kernel/rcswitch/command

	# turn switch on
	echo "11111A1" > /sys/kernel/rcswitch/command


Credits:

Most of the parts for sending command to the RCSwitches over RF are
taken from r10r [rcswitch-pi] (https://github.com/r10r/rcswitch-pi)


Loading the module on the target
--------------------------------

To load the kernel module at bootup, add the following to
`/etc/rc.local` (before the `exit 0` statement):

	insmod /opt/rcswitch/module/rcswitch.ko

The module knows the following parameters:

	tx_gpio		- Number of GPIO to which TX of 433Mhz sender is connected (default 9/CTS). (int)

	en_gpio		- Number of GPIO to which 3v3 of 433Mhz sender is connected (default 7/RTS). (int)
			  Note: use -1 to disable use of enable GPIO.

	pulse_duration	- Duration of a single pulse in usec. (default 350) (int)

	tx_repeat	- Number of how many times a message is repeated (default 10). (int)


E.g. if connected to CTS/RTS on the Carambola:

	insmod /opt/rcswitch/module/rcswitch.ko tx_gpio=9 en_gpio=7

Or if TX is connected to pin 17 on RasPi, and Vcc to 5V:

	insmod /opt/rcswitch/module/rcswitch.ko tx_gpio=17 en_gpio=-1


Usage
-----

The module exports two entries to the sysfs (under `/sys/kernel/rcswitch`):

	command - Write Only. Accepts commands as defined below.

	power   - Read/Write. If the module was given a "en_gpio"i writing to `power`
            could be used to enable (1)/disable (0) the RF module. Reading from
 	          `power` will return the current state (1=enabled, 0=disabled).


The command format is defined as follows:

A command string has teh format: AAAAACS

 Where:

    AAAAA  - address bits as set on the outlets DIP switches - e.g. '11111'
    C      - channel A, B, C or D as set on the outlets DIP switches - e.g. 'A'
             instead of A, B, C, D one could also use 1, 2, 3 or 4
    S      - state 1 (on) or 0 (off) - e.g. '1'

Complete command string examples:

    '11111A0' - Switch channel A off for address '11111'
    '1111111' - Same as above
    '11111B1' - Switch channel B on  for address '11111'
    '1111121' - Same as above

Commands are send to through the kernel modules sysfs entry at: /sys/kernel/rcswitch/command

Examples:

    echo "11111A0" > /sys/kernel/rcswitch/command
    echo "11111A1" > /sys/kernel/rcswitch/command

