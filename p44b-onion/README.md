# p44b umbrella package aimed at building OnionOS with p44b

Once completed, this package should make possible building the OnionOS variant
of OpenWrt on top of a unmodified OpenWrt tree with something like:

```bash
cd /my/openwrt/tree
/path/to/p44build/p44b init feeds/plan44/p44b-onion/p44build
./p44b prepare
./p44b target onion-omega2
./p44b build
./p44b package
```

or even just

```bash
./p44b buildtargets ALL
```

## WARNING: THIS IS WORK IN PROGRESS - NOT YET WORKING AT ALL AS-IS

## Current status

At this time, the only valuable part are the contents of `p44build/global-patches`, which are patches (patch number in paranthesis) to the OpenWrt v22.03.5 tree providing:

- hardware PWM (204)
- I2S sound (210)
- runtime loadable device tree overlays (251..253)
- Modern GPIO via cdev support (252)
- re-introducing mt7621 GPIO "base" of-property to keep traditional GPIO numbering (257, 281)
- and a few minor package fixes (900..906)

In particular, loadable device tree is a big deal, as it gives a lot of flexibility of configuring hardware and also a replacement for xxx-gpio-custom which are no longer supported in newer kernels.

Also, retaining GPIOs numbered after the actual Pin numbers on the SoC is something I think is important, and should also be argued for in OpenWrt itself (I [tried](https://forum.openwrt.org/t/state-of-userland-access-to-gpio-based-hardware/130505) but the IoT OpenWrt community is not very established yet among OpenWrt core devs...)

## Missing

- Package makefile to actually install/configure/make this package.
- adapted `scripts/*` for building, packaging etc. according to OnionOS's needs.
- OnionOS side patches (the one present now are plan44 additions, that *might* be useful for Onion, but are not yet officially included)

## Future

I hope this package can become an easy way for occasionally building OnionOS firmware, possibly slightly modified, on your own OpneWrt working tree, side-by-side with other OpenWrt based firmware.

With Onion having thankfully separated all the needed tree patches and configs into the [openwrt-buildsystem-wrapper](https://github.com/OnionIoT/openwrt-buildsystem-wrapper), I think this should become possible soon!
