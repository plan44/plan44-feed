# Copyright (c) 2014 Stefan Wendler <sw@kaltpost.de>
#
# Original project: https://github.com/wendlers/rcswitch-kmod
#
# Extracted kernel module and packaged as a independent OpenWrt module
# by Lukas Zeller <luz@plan44.ch> in 2018
#
# Please see README.md for information and visit the


include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

# name
PKG_NAME:=rcswitch
# version of what we are downloading
PKG_VERSION:=1.0
# version of this makefile
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)
PKG_CHECK_FORMAT_SECURITY:=0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
	SUBMENU:=Other modules
	TITLE:=driver for RCSwitch 434Mhz radio protocol
	FILES:= $(PKG_BUILD_DIR)/rcswitch.ko
endef

define KernelPackage/$(PKG_NAME)/description
	This package contains the rcswitch kernel driver which generates the timing for
  driving 434Mhz radio modules used to remote control cheap radio controlled
  power plug/socket switches.
  This OpenWrt/LEDE package is a kernel-module-only extract from the original project
  Stefan Wendler <sw@kaltpost.de>, hosted at https://github.com/wendlers/rcswitch-kmod
  Please see original project for client samples in bash and python as well
  as documentation.
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
