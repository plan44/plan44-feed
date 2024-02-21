# Copyright (C) 2024 Lukas Zeller <luz@plan44.ch>
# See LICENSE for more information.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

# name
PKG_NAME:=p44-stepper
# version of what we are downloading
PKG_VERSION:=1
# version of this makefile
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)
PKG_CHECK_FORMAT_SECURITY:=0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
	SUBMENU:=Other modules
	TITLE:=p44-stepper driver for GPIO based stepper motor driving
	FILES:= $(PKG_BUILD_DIR)/p44-stepper.ko
endef

define KernelPackage/$(PKG_NAME)/description
	This package contains the p44-stepper kernel driver for driving stepper
	motors connected to GPIO outputs
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
