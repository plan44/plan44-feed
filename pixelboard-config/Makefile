# Copyright (c) 2016 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=pixelboard-config
# version of what we are downloading
PKG_VERSION:=0.2
# version of this makefile
PKG_RELEASE:=2

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_CHECK_FORMAT_SECURITY:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	DEPENDS:=+mg44 +pixelboardd
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=toys
	TITLE:=PIXELBOARD configuration setup
	URL:=https://plan44.ch/automation
	MAINTAINER:=luz@plan44.ch
	MENU:=1
endef


define Package/$(PKG_NAME)/description
 contains the configuration setup for PIXELBOARD
endef


define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef


# derive P44 platform identifier
ifeq ($(CONFIG_TARGET_brcm2708_bcm2708_RaspberryPi),y)
  PIXELBOARD_CONFIG_PLATFORM_IDENTIFIER:=raspberry-pi-1
endif
ifeq ($(CONFIG_TARGET_ar71xx_generic_OMEGA),y)
  PIXELBOARD_CONFIG_PLATFORM_IDENTIFIER:=onion-omega
endif
ifeq ($(CONFIG_TARGET_ar71xx_generic_CARAMBOLA2),y)
  PIXELBOARD_CONFIG_PLATFORM_IDENTIFIER:=carambola2
endif
ifeq ($(CONFIG_TARGET_ramips_mt7688_DEVICE_LinkIt7688),y)
  PIXELBOARD_CONFIG_PLATFORM_IDENTIFIER:=lis7688
endif
ifeq ($(CONFIG_TARGET_ramips_mt7688_DEVICE_P44DSBOmega2),y)
  PIXELBOARD_CONFIG_PLATFORM_IDENTIFIER:=p44-dsb-omega2
endif


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/etc
	$(CP) ./files/* $(PKG_BUILD_DIR)/
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/flash
	$(INSTALL_DIR) $(1)/www
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/config_p44defaults
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/lib/upgrade/keep.d
	$(CP) $(PKG_BUILD_DIR)/etc/* $(1)/etc
	$(CP) $(PKG_BUILD_DIR)/config_p44defaults_${PIXELBOARD_CONFIG_PLATFORM_IDENTIFIER}/* $(1)/etc/config_p44defaults
	$(CP) $(PKG_BUILD_DIR)/config_p44defaults_${PIXELBOARD_CONFIG_PLATFORM_IDENTIFIER}/* $(1)/etc/config
	$(CP) $(PKG_BUILD_DIR)/www/* $(1)/www
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sbin/* $(1)/sbin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/keep-pixelboard $(1)/lib/upgrade/keep.d/pixelboard
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
