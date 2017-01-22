MessageTorch OpenWrt package
============================

[![Flattr this git repo](http://api.flattr.com/button/flattr-badge-large.png)](https://flattr.com/submit/auto?user_id=luz&url=https://github.com/plan44/messagetorch-openwrt-package&title=MessageTorch&language=&tags=github&category=software)

**This is a (currently quick&dirty) port of the *messagetorch* project to OpenWrt for the Omega Onion.

The project originated on [particle.io spark core](https://www.particle.io) - see details in [this particle forum post](https://community.particle.io/t/messagetorch-torch-fire-animation-with-ws2812-leds-message-display/2551) and in the [sources on github](https://github.com/plan44/messagetorch).

![Hello Spark](MessageTorch.gif)

For running *messagetorch* on [Onion Omega](https://onion.io/store/), you need to install the *ws2812-draiveris* kernel driver ([source on github](https://github.com/jnweiger/ws2812_sprites/tree/master/carambola2/package/ws2812_draiveris/src)). To allow experimenting for those Omega users who don't have a OpenWrt buildroot, [here's a installable package](http://plan44.ch/downloads/experimental/kmod-ws2812-draiveris_3.18.29%2B0.1-9_ar71xx.ipk). [Update: For those who want to experiment with the SK6812 RGBW LEDs, there's a [newer version of the ws2812-draiveris driver](http://plan44.ch/downloads/experimental/kmod-ws2812-draiveris_4.1.23%2B0.1-9_ar71xx.ipk), which supports SK6812 with the "rgbw" flag)

You can install it from the Onion console (--force-depends may be needed when kernel version does not match exactly -> experimental, might crash, make backups!)

    cd /tmp
    wget http://plan44.ch/downloads/experimental/kmod-ws2812-draiveris_3.18.29%2B0.1-9_ar71xx.ipk
    opkg install /tmp/kmod-ws2812-draiveris_3.18.29+0.1-9_ar71xx.ipk --force-depends

To activate the driver for 256 LEDs with data pin connected to GPIO7:

    insmod ws2812-draiveris gpios=7 leds_per_gpio=256 # for WS2812
    insmod ws2812-draiveris gpios=7 leds_per_gpio=256 rgbw=1 # for SK6812
    
Now you can install messagetorch (again, [installable package](http://plan44.ch/downloads/experimental/messagetorch_0.1-1_ar71xx.ipk) available for convenience)
    
    cd /tmp
    wget http://plan44.ch/downloads/experimental/messagetorch_0.1-1_ar71xx.ipk
    opkg install /tmp/messagetorch_0.1-1_ar71xx.ipk

Assuming a "torch" with 18 windings of 13 LEDs each, start the torch with

    messagetorch -W 13 -H 18 "Hello Omega"

Once running, you can remote control it by sending UDP packets. The following is from a Mac OS X command line (nc options might vary on other platforms, nc on omega itself does not have the -u option):

	echo -n 'New Text' | nc -w 1 -u my.omegas.ip.address 4442
	
Or you can change many parameters, for example the mode

	echo -n '/param mode=2' | nc -w 1 -u my.omegas.ip.address 4442

See function *handleParams()* in the source code for supported parameters. Most of them are for tuning the fire animation and text display, but there's also a configurable clock:

	echo -n '/param clock_interval=300' | nc -w 1 -u my.omegas.ip.address 4442
	
This will show the current time as a scrolling message once every 5 minutes (300 seconds).

Have fun!

