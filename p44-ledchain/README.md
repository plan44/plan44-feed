p44-ledchain for MT7688
=======================

This driver uses MT7688 hardware PWM and DMA to generate WS2812 timing
## Please note

## Using the driver

after compiling/installing the p44-ledchain kernel module package, activate the driver as follows:

	# example module load for 20 WS2812 LEDs connected to PWM1:
	insmod p44-ledchain pwm_channel=1 inverted=1
	# test - should turn on the first LED in blue color
	# (each LED has 3 bytes: R,G,B)
	echo -en '\x00\x00\x40' > /dev/ledchain

	# example module load for 20 SK6812 LEDs:
	insmod ws2812-draiveris pwm_channel=1 inverted=1 rgbw=1
	# test - should turn on the first LED in blueish white color
	# (each LED has 4 bytes: R,G,B,W)
	echo -en '\x00\x00\x40\x40' > /dev/ledchain
