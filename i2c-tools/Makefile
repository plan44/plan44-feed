#
# Copyright (C) 2007-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General ic License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=i2c-tools
PKG_VERSION:=3.1.2
PKG_RELEASE:=2

PKG_SOURCE_URL:=http://dl.lm-sensors.org/i2c-tools/releases/ \
                http://fossies.org/linux/misc/

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_HASH:=db5e69f2e2a6e3aa2ecdfe6a5f490b149c504468770f58921c8c5b8a7860a441

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=Utilities
	TITLE:=I2C tools for Linux
	URL:=https://github.com/plan44/i2c-tools
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 This package contains set of I2C command line tools for Linux.
 These were originally part of the lm-sensors package.

 This version was copied from the onion.io version
 and has added a Build/InstallDev step to install the enhanced
 i2c-dev.h header (which overrides the standard i2c-dev.h
 system header) into the build environment, so other
 packages can use it.

 Note that it also installs a copy of i2c-dev.h as lib-i2c-dev.h
 because due to search path order, the override might not work
 when the toolchain /usr/include also contains a i2c-dev.h.

 Note also that the originally included python-smbus package has
 been removed from p44 version of i2c-tools
endef

TARGET_CPPFLAGS += -I$(PKG_BUILD_DIR)/include

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		LINUX="$(LINUX_DIR)" \
		CC="$(TARGET_CC)" \
		STAGING_DIR="$(STAGING_DIR)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		CFLAGS="$(TARGET_CFLAGS)"
endef


define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/linux
	$(CP) $(PKG_BUILD_DIR)/include/linux/i2c-dev.h $(1)/usr/include/linux
	$(CP) $(PKG_BUILD_DIR)/include/linux/i2c-dev.h $(1)/usr/include/linux/lib-i2c-dev.h
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/i2cdetect $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/i2cdump $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/i2cset $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/i2cget $(1)/usr/sbin/
endef


$(eval $(call BuildPackage,$(PKG_NAME)))

