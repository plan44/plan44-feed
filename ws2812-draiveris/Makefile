# Copyright (C) 2012 Žilvinas Valinskas, Saulius Lukšė
# Copyright (C) 2014 Jürgen Weigert <jw@owncloud.com>
# See LICENSE for more information.
#
# Forked from https://github.com/Lukse/carambola2_ws2812_driver
#
# References: http://www.adafruit.com/datasheets/WS2812.pdf
#
## PC Build single package
# cd /home/test/carambola2/
# clear; make package/ws2812-draiveris/clean
# clear; make package/ws2812-draiveris/compile V=99

# PC: Carambola picocom
# sudo picocom -b 115200 /dev/ttyUSB0

# Carambola: get packet
## rmmod ws2812-draiveris;
## rm /lib/modules/3.7.9/ws2812-draiveris.ko;
## cd /tmp; rm kmod-ws2812-draiveris*;
## wget http://192.168.0.63:8000/packages/kmod-ws2812-draiveris_3.7.9-5_ar71xx.ipk;
## opkg install kmod-ws2812-draiveris_3.7.9-5_ar71xx.ipk --force-reinstall;
## insmod /lib/modules/3.7.9/ws2812-draiveris.ko gpio_number=20 inverted=1
## mknod /dev/ws2812 c 253 0
## echo -en '\x00\x00\x40' > /dev/ws2812

# test.py
##dev = os.open("/dev/ws2812", os.O_RDWR)
##data = [0x0, 0x60, 0x00]
##os.write(dev, bytearray(data))

# upgrade firmware
## cd /tmp; wget http://192.168.0.63:8000/openwrt-ar71xx-generic-carambola2-squashfs-sysupgrade.bin; sysupgrade -v -n openwrt-ar71xx-generic-carambola2-squashfs-sysupgrade.bin

# carambola install python
## opkg update
## opkg install python

# Copyright (C) 2006-2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

# name
PKG_NAME:=ws2812-draiveris
# version of what we are downloading
PKG_VERSION:=0.1
# version of this makefile
PKG_RELEASE:=9

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)
PKG_CHECK_FORMAT_SECURITY:=0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
	SUBMENU:=Other modules
	TITLE:=ws2812 draiveris drivers
	FILES:= $(PKG_BUILD_DIR)/ws2812-draiveris.ko
	#AUTOLOAD:=$(call AutoLoad, 92, ws2812-draiveris-dev)
endef

define KernelPackage/$(PKG_NAME)/description
	This package contains ws2812-draiveris drivers for WS2812 LED chains
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)"

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,$(PKG_NAME)))
