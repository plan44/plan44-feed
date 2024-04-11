# Copyright (C) 2024 Lukas Zeller <luz@plan44.ch>
# See LICENSE for more information.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

# name
PKG_NAME:=p44-counter
# version of what we are downloading
PKG_VERSION:=1.2
# version of this makefile
PKG_RELEASE:=3

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)
PKG_CHECK_FORMAT_SECURITY:=0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
	SUBMENU:=Other modules
	TITLE:=p44-counter driver for GPIO based pulse counting
	FILES:= $(PKG_BUILD_DIR)/p44-counter.ko
endef

define KernelPackage/$(PKG_NAME)/description
	This package contains the p44-counter kernel driver for counting pulses
	at GPIO inputs.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


MAKE_OPTS:= \
	$(KERNEL_MAKE_FLAGS) \
	M="$(PKG_BUILD_DIR)"

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,$(PKG_NAME)))
