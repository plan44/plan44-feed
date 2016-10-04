# Copyright (C) 2012 Žilvinas Valinskas, Saulius Lukšė
# Copyright (C) 2014 Jürgen Weigert <jw@owncloud.com>
# Copyright (C) 2016 Lukas Zeller <luz@plan44.ch> (RGBW support)
# See LICENSE for more information.
#
# Forked from https://github.com/jnweiger/ws2812_sprites
# which was forked from https://github.com/Lukse/carambola2_ws2812_driver
#
# References: http://www.adafruit.com/datasheets/WS2812.pdf
# References: https://cdn-shop.adafruit.com/product-files/1138/SK6812+LED+datasheet+.pdf
#

## === example module load for WS2812:
## insmod ws2812-draiveris gpio_number=20 inverted=1
## echo -en '\x00\x00\x40' > /dev/ws2812

## === example module load for SK6812:
## insmod ws2812-draiveris gpio_number=20 inverted=1 rgbw=1
## echo -en '\x00\x00\x40\x20' > /dev/ws2812

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
	This package contains ws2812-draiveris drivers for WS2812/SK6812 LED chains
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
