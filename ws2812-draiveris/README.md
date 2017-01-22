ws2812-draiveris AR9331 driver
==============================

This WS2812 driver package was extracted from a subdir of the [ws2812\_sprites](https://github.com/jnweiger/ws2812_sprites) project, which in turn contained a fork of the [carambola2\_ws2812\_driver](https://github.com/Lukse/carambola2_ws2812_driver).

I extracted the driver to get WS2812 support on the Onion Omega1 (also a AR9331 based module), and modified it to support the SK6812 RGBW LEDs as well. [vdcd](https://github.com/plan44/vdcd) and [messagetorch](https://github.com/plan44/messagetorch-openwrt-package) are two of my project that make use of this driver.

## Please note

While the driver works fine as such, it is *not* a good Linux kernel space citizen, as it needs to block IRQs for extended time periods (this is unfortunately unavoidable to generate the WS2812 timing on the AR9331 which lacks DMA-controlled PWM/timer outputs). So please be aware that writing to /dev/ws2812 will probably delay or otherwise mess up any other tightly timed operations.

## Using the driver

after compiling/installing the ws2812-draiveris kernel module package, activate the driver as follows:
	
	# example module load for 20 WS2812 LEDs:
	insmod ws2812-draiveris gpio_number=20 inverted=1
	# test - should turn on the first LED in blue color
	# (each LED has 3 bytes: R,G,B)
	echo -en '\x00\x00\x40' > /dev/ws2812

	# example module load for 20 SK6812 LEDs:
	insmod ws2812-draiveris gpio_number=20 inverted=1 rgbw=1
	# test - should turn on the first LED in blueish white color
	# (each LED has 4 bytes: R,G,B,W)
	echo -en '\x00\x00\x40\x40' > /dev/ws2812
