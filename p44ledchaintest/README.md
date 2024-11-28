p44ledchaintest
===============

This is a small utility indended for testing the p44-ledchain smart LED chain kernel driver for MT7688 SoC (and possibly other smart LED chain drivers using the same interface).

(c) 2017-2024 by luz@plan44.ch

## Usage

To get help, just type

    p44ledchaintest -h

The simplest way to use the utility is just providing one (or multiple) led devices. In this mode, up to 720 LEDs will be set to bright red:

    p44ledchaintest /dev/ledchain0

**Note**: if the p44-ledchain driver is in *variable led type mode*, the data stream always needs to contain a header specifying the led type parameters. See p44-ledchain README for details. This header can be sent with *p44ledchaintest* using the `-H` option:

    p44ledchaintest -H 0203000000 /dev/ledchain0

(0203000000 is the header for WS2813 GRB chains)
For the following examples, I do not show the -H option, but if p44-ledchain modules is initialized in *variable led type mode*, it needs to be included.

You can set a different color using the -c option

    p44ledchaintest -c FFEE00  /dev/ledchain0

To stress test a led chain (to spot occasional flickering e.g. when the tmaxpassive time is too long for old WS2812 chains etc.), *p44ledchaintest* can do repeated updates using the `-r` and `-i` options.

To get more interesting visual output than just a single color, there are the `-S` (single wandering LED) and `-F` (fill up).

Instead of the default colors (blue background, red foreground), `-c rrggbb` and `-b rrggbb` can be used to specify other colors. `-s rrggbb` can be used to add a color increment at each step.

So to have a light blue dot wandering over a pink background of the first 50 leds, updating every 25mS and repeating forever (until you hit `^C`) you could type:

    p44ledchaintest -n 50 -r 0 -i 25 -c 0055FF -b 330033 -S /dev/ledchain0

To see some statistics about timing use the `-v` option.

### 4-channel RGBW LEDs (such as SK6812)

Use `-4` for 4-channel LEDs. With these, the color options (`-c`, `-b`, `-s`) take a fourth byte representing the white channel.

### 16bit resolution LEDs (such as WS2816)

Use `-2` for LED which have 16 bit resolution, i.e. 2 bytes per color component.

The color options (`-c`, `-b`, `-s`) still remain 8bit, but you can use the `-g` option to convert the 8-bit color value into a 16-bit PWM value with better perceived brightness resolution at low levels.

Without `-g`, the 8-bit color value will just be multiplied by 256 for 16-bit LEDs.
