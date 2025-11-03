# OpenWrt packaging (C) 2025 by Lukas Zeller <luz@plan44.ch>
# dtbocfg driver (c) 2016-2017, Ichiro Kawazome
#   See src/LICENSE and src/Readme.md for more information.
#
# Original project: https://github.com/ikwzm/dtbocfg
#
# This is free software, see src/LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

# name
PKG_NAME:=dtbocfg
# version of what we are downloading
PKG_VERSION:=0.1.1
# version of this makefile
PKG_RELEASE:=2

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)
PKG_CHECK_FORMAT_SECURITY:=0

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
	SUBMENU:=Other modules
	TITLE:=Device Tree Blob Overlay Configuration File System
	FILES:= $(PKG_BUILD_DIR)/dtbocfg.ko
	# the trailing 1 arg causes early load from /etc/modules-boot.d instead of later from /etc/modules.d
	AUTOLOAD:=$(call AutoLoad,21,$(PKG_NAME),1)
endef

define KernelPackage/$(PKG_NAME)/description
	This package contains the dtbocfg kernel driver which allows device tree blob overlay
	configuration from user space via sysfs.
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

define KernelPackage/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/lib/preinit
	# name/number controls order in preinit
	$(INSTALL_BIN) ./files/90_apply_dt_overlays $(1)/lib/preinit/90_apply_dt_overlays
	# dtoverlay utility for quickly testing overlays
	$(INSTALL_BIN) ./files/dtoverlay $(1)/sbin
endef

$(eval $(call KernelPackage,$(PKG_NAME)))
