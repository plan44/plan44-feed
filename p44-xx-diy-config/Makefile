# Copyright (c) 2016-2025 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44-xx-diy-config
# version of what we are downloading
PKG_VERSION:=1.8.2.42
# version of this makefile
PKG_RELEASE:=5

# derive P44 platform identifier
ifeq ($(CONFIG_TARGET_ramips_mt76x8_DEVICE_plan44_p44dsbomega2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=p44xx-omega2
endif
ifeq ($(CONFIG_TARGET_bcm27xx_bcm2708_DEVICE_rpi),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=raspberry-pi-1
endif
ifeq ($(CONFIG_TARGET_bcm27xx_bcm2709_DEVICE_rpi-2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=raspberry-pi-2
endif


PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)-$(P44_CONFIG_PLATFORM_IDENTIFIER)
PKG_CHECK_FORMAT_SECURITY:=0

PKG_CONFIG_DEPENDS := \
	CONFIG_VERSION_NUMBER \
	CONFIG_P44_FEED_NAME

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	DEPENDS:=+p44pagekite +mg44 +vdcd +p44maintd
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=maker
	TITLE:=P44-xx configuration setup
	URL:=https://plan44.ch/automation
	MAINTAINER:=luz@plan44.ch
	MENU:=1
endef


define Package/$(PKG_NAME)/description
 contains the configuration setup for the P44-xx
endef


define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef


define Build/Prepare
	# copy makefile into build dir
	$(CP) ./files/Makefile $(PKG_BUILD_DIR)/
	# copy common files for all P44 platforms
	$(CP) ./files/common/* $(PKG_BUILD_DIR)/
	# copy files specific to this P44 platform
	$(CP) ./files/$(P44_CONFIG_PLATFORM_IDENTIFIER)/* $(PKG_BUILD_DIR)/
	# append identifier to main platform definition
	echo "PLATFORM_IDENTIFIER=\"$(P44_CONFIG_PLATFORM_IDENTIFIER)\"" >> $(PKG_BUILD_DIR)/etc/p44platform.defs
	# append operating system
	echo "PLATFORM_OS_IDENTIFIER=\"openwrt\"" >> $(PKG_BUILD_DIR)/etc/p44platform.defs
	# create version file
	echo $(CONFIG_VERSION_NUMBER) > $(PKG_BUILD_DIR)/etc/p44version
	# create feed name file
	echo $(CONFIG_P44_FEED_NAME) > $(PKG_BUILD_DIR)/etc/p44feed
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/flash
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/usr/share/p44xx
	$(INSTALL_DIR) $(1)/www
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/lib/upgrade/keep.d
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(CP) $(PKG_BUILD_DIR)/etc/* $(1)/etc
	$(CP) $(PKG_BUILD_DIR)/usr/* $(1)/usr
	$(CP) $(PKG_BUILD_DIR)/lib/* $(1)/lib
	$(CP) $(PKG_BUILD_DIR)/www/* $(1)/www
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sbin/* $(1)/sbin
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
