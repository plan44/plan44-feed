# dtbocfg - Device Tree Blob Overlay Configuration File System

This is an OpenWrt package for the dtbocfg kernel driver.
To reduce dependencies on platforms, the small amount of source code is included in the package

See https://github.com/ikwzm/dtbocfg for upstream project
See src/Readme.md for info about the project and instructions how to use the driver

- dtbocfg driver (c) 2016-2017, Ichiro Kawazome <ichiro_k@ca2.so-net.ne.jp>
- OpenWrt packaging (C) 2025 by Lukas Zeller <luz@plan44.ch>

## Notes
- the `90_apply_dt_overlays` preinit script will load overlays from `/lib/firmware/device-tree/overlays`
  at preinit time, which is the same mechanism the OpenWrt patch set based on Pantelis Antoniou's kernel
  patches did. The idea is that overlays needed for operation can be stored (or soft-linked) there to become
  active early after boot.
- a helper tool named `dtoverlay` is included to help compiling and installing overlays on the fly
  for testing/debugging. It is very basic at this time, plan is to add support for all dt overlay
  related tasks over time.
