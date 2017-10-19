# plan44 feed for OpenWrt/LEDE

This is my source feed for building various LEDE/OpenWrt packages I wrote for projects on Onion Omega1 and Omega2.

If you have a LEDE or OpenWrt build environment set up, you can use the plan44 feed by adding the line

    src-git plan44 ssh://plan44@plan44.ch/home/plan44/sharedgit/plan44-public-feed.git;master

to your `feeds.conf.default` (or, more correctly, to a copy of `feeds.conf.default` named `feeds.conf`)

Note: the master branch of this feed is intended to use with the current LEDE release (17.01 at the time of writing). If you want to build LEDE master, use `for-lede-master` version of the feed.


## Useful right now

The following packages are of general usefulness right now:

- **p44-ledchain**: this is a kernel module written by me for the MT7688/Omega2 which makes use of the hardware PWM in the chip to drive individually addressable WS281x-type LED chains. In contrast to the *ws2812_draiveris* driver (see below) that does the same for AR9331/Omega1, *p44-ledchain* does **not** block IRQs at all. At this time, *p44-ledchain* is extensively tested with a chain of 200 WS2813, and one of 24 P9823 LEDs. The driver also has modes for WS2811, WS2812 and the RGBW SK6812 LEDs.

- **pixelboard-config**: this is a "master" package containing all information needed to build the firmware for *pixelboard* (tetris/game-of-life lounge table based on WS2813 LEDs). See pixelboard-config/README.md for detailed instructions how to build the FW image.

- **pagekitec** (also containing **libpagekite**): copied this from [remakeelectric/owrt_pub_feeds](https://github.com/remakeelectric/owrt_pub_feeds), a C implementation of the [pagekite](https://pagekite.net) reverse proxy protocol which is useful to make services of devices which do not have a public IP or are behind firewalls accessible from the internet.

- **ws2812-draiveris**: this is a WS281x LED driver that works for AR9331/Omega1, but blocks interrupts for several milliseconds per update, which is *very* bad behaviour for a kernel module. The author of *ws2812-draiveris* (not me) is not to blame for this, because the AR9331 chip does not provide any hardware that could generate WS281x timing, so bitbanging and blocking IRQs is the *only* way to do WS281x on a AR9331. Still, it's a bit of a hack that could backfire when other parts of the system rely on fast IRQ response.


## Work in progress!

At this time, there's also a lot of work-in-progress in this feed, so don't expect everything turnkey ready. In particular, the p44sbbd, p44sbb-config, pixelboard and pixelboard-config packages are parts of ongoing projects not yet ready for prime time.

Note that *i2c-tools* and *libpng* are also available in the standard feeds, but in slightly different variations - I needed some modifications for my own projects so I duplicated the packages. Fortunately, OpenWrt/LEDE source feed management is prepared for having multiple versions of the same package - just use the -p option to choose the feed to install a particular package from:

    ./scripts/feeds update plan44
    ./scripts/feeds install -p plan44 libpng


