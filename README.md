# plan44 feed for OpenWrt

This is my source feed for building various OpenWrt packages I wrote for projects on Onion Omega1 and Omega2.

If you have a OpenWrt build environment set up, you can use the plan44 feed by adding the line

    src-git plan44 https://github.com/plan44/plan44-feed.git;master

to your `feeds.conf.default` (or, more correctly, to a copy of `feeds.conf.default` named `feeds.conf`).

## Useful right now

The following packages are of general usefulness right now:

- **p44mbrd**: a openwrt package for using the [p44mbrd](https://github.com/plan44/p44mbrd) *matter* bridge daemon on OpenWrt (usually as a companion to [vdcd](https://github.com/plan44/vdcd)).

- **p44-counter**: a simple kernel module for counting pulses on GPIOs, including quadrature encoder signals. Will become obsolete once drivers based on newer kernel's counter infrastructure become available, but that's not yet the case for my current use cases/platforms/versions.

- **p44-ledchain**: this is a kernel module written by me for the MT7688/Omega2 which makes use of the hardware PWM in the chip to drive individually addressable WS281x-type LED chains. In contrast to the *ws2812_draiveris* driver (see below) which does the same for AR9331/Omega1, *p44-ledchain* does **not** block IRQs at all. At this time, *p44-ledchain* is extensively tested with up to 4 chains with ~900 WS2815 or WS2813 each. The driver also has modes for other chips like WS2811, WS2812, RGBW SK6812 and different LED color layouts like RGB, GRB etc.

- **pagekitec**: a package containing the C implementation of pagekite, which is a reverse proxy that allows reaching embedded targets from the internet (e.g. for service, maintainance, monitoring). Have a look at pagekite.net for information (pagekite is available as a hosted service but also provides the server side as a oppen source python script you can use on your own server)

- **serialfwd**: a small utility I use in almost every project which allows to access a serial interface via TCP from a remote host, and also to send and receive a few hex bytes directly.

- **ws2812-draiveris**: this is a WS281x LED driver that works for AR9331/Omega1, but blocks interrupts for several milliseconds per update, which is *very* bad behaviour for a kernel module. The author of *ws2812-draivris* (not me) is not to blame for this, because the AR9331 chip does not provide any hardware that could generate WS281x timing, so bitbanging and blocking IRQs is the *only* way to do WS281x on a AR9331. Still, it's a bit of a hack that could backfire when other parts of the system rely on fast IRQ response.

- **rcswitch**: an extract of the rcswitch kernel module for use in OpenWrt from a bigger project. rcswitch provides a sysfs interface for a connected 434MHz RF module, controlling simple RF power switches.

## Complete projects

The **\*-config** packages are "umbrella" packages to be used with the [p44b script](https://github.com/plan44/p44build) for various projects of mine. The older projects are still based on OpenWrt 19.07, 18.06 or even 17.x, recently (re-)built projects are based on OpenWrt 22.03.5 or later.

These config packages include everything needed to build the project based on a clean openwrt tree, in particular the subdirectory `p44build/global-patches` contains patches for all changes in the openwrt tree needed to build the project.

The [p44b script](https://github.com/plan44/p44build) uses quilt to apply these meta-patches when preparing a project.

The idea behind these patches is to avoid forking the openwrt tree, but have the set of needed changes clearly documented in the umbrella *-cofig package.
The `global-patches` can also be useful resources to port improvements for the Omega2S (such as working i2S ASoC) to other projects.

- **p44-xx-diy-config**: this is the "umbrella" package that contains everything needed to build installable firmware images to create a full featured P44-XX-DIY standalone SmartLED/DMX/automation controller on an Onion Omega2 or a RaspberryPi 1..4. See [p44-xx-open](https://github.com/plan44/p44-xx-open) for more information.

- **p44ttngw**: a port of a The Things Network (TTN) LoRA gateway to the Omega2, for use with the RAK831 radio.
- **pixelboard**: a lounge table with a Omega2 based tetris and game of life built into the table plate itself.
- **p44sbb**: a clock based on Swiss railway splitflap displays
- **p44wiper**: a DC motor controller for smooth "wiping" movements of a DC motor with a single reed contact to avoid drift.
- **p44bandit**: a serial interface controller for a old BANDIT CN milling machine.
- **hermel** and **leth**: firmware for a Omega2S based controller using the PWM outputs for driving WS2813 LED chains for some effects and a scrolling text ticker, and a 2-channel A/D converter for sensor input, used for exhibition objects.
- **hmt20**: firmware for a Omega2S based controller which allows connecting up to 24 cheap NXP MFRC522 based RFID reader boards and up to 4 WS281x LED chains, used for exhibition objects.

The **\*d** packages (vdcd, p44mbrd, pixelboard, p44featured, ...) are openwrt makefiles for the linux daemon applications that usually are the core part of one of the above projects.

There are also some ports to openwrt for things I wanted to use for a project, e.g.

- **pagekitec**: C version of the pagekite reverse proxy protocol (for remote access to devices via a pagekite server)
- **hxcmodplayer**: a tiny MOD player requiring very little resources, working with ASoC
- **timidity**: a MIDI expander, compiling, but still WIP (does not seem to work with ASoC yet)


## Work in progress!

There is and will always be some work-in-progress in this feed, so don't expect everything turnkey ready.
