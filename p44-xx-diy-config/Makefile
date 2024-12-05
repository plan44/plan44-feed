# Copyright (c) 2016-2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44-xx-diy-config
# version of what we are downloading
PKG_VERSION:=1.8.1.8
# version of this makefile
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
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


# derive P44 platform identifier
ifeq ($(CONFIG_TARGET_ramips_mt76x8_DEVICE_plan44_p44dsbomega2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=omega2
endif
ifeq ($(CONFIG_TARGET_bcm27xx_bcm2708_DEVICE_rpi),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=raspberry-pi-1
endif
ifeq ($(CONFIG_TARGET_bcm27xx_bcm2709_DEVICE_rpi-2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=raspberry-pi-2
endif


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/etc
	$(CP) ./files/* $(PKG_BUILD_DIR)/
	# copy main platform definition file (more platform defs might be loaded dynamically)
	$(CP) ./platform_defs/$(P44_CONFIG_PLATFORM_IDENTIFIER).defs $(PKG_BUILD_DIR)/etc/p44platform.defs
	# - append identifier
	echo "PLATFORM_IDENTIFIER=\"$(P44_CONFIG_PLATFORM_IDENTIFIER)\"" >> $(PKG_BUILD_DIR)/etc/p44platform.defs
	# - append operating system
	echo "PLATFORM_OS_IDENTIFIER=\"openwrt\"" >> $(PKG_BUILD_DIR)/etc/p44platform.defs
	# copy all platform and product definitions that might be dynamically loaded depending on uboot env settings
	$(CP) ./platform_defs/p44* $(PKG_BUILD_DIR)/etc
	$(CP) ./product_defs/p44* $(PKG_BUILD_DIR)/etc
	# create version file
	echo $(CONFIG_VERSION_NUMBER) > $(PKG_BUILD_DIR)/etc/p44version
	# create feed name file
	echo $(CONFIG_P44_FEED_NAME) > $(PKG_BUILD_DIR)/etc/p44feed
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/flash
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/config_p44defaults
	$(INSTALL_DIR) $(1)/usr/share/p44xx
	$(INSTALL_DIR) $(1)/www
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/lib/upgrade/keep.d
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(CP) $(PKG_BUILD_DIR)/etc/* $(1)/etc
	$(CP) $(PKG_BUILD_DIR)/config_p44defaults_${P44_CONFIG_PLATFORM_IDENTIFIER}/* $(1)/etc/config_p44defaults
	$(CP) $(PKG_BUILD_DIR)/config_p44defaults_${P44_CONFIG_PLATFORM_IDENTIFIER}/* $(1)/etc/config
	$(CP) $(PKG_BUILD_DIR)/resources/* $(1)/usr/share/p44xx
	$(CP) $(PKG_BUILD_DIR)/www/* $(1)/www
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sbin/* $(1)/sbin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/keep-p44xx_${P44_CONFIG_PLATFORM_IDENTIFIER} $(1)/lib/upgrade/keep.d/p44xx
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/ubus-acls/* $(1)/usr/share/rpcd/acl.d
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
