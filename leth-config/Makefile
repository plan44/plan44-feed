# Copyright (c) 2018 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=leth-config
# version of what we are downloading
PKG_VERSION:=0.4
# version of this makefile
PKG_RELEASE:=4

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_CHECK_FORMAT_SECURITY:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	DEPENDS:=+mg44 +lethd
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=custom
	TITLE:=P44-WIPER configuration setup
	URL:=https://plan44.ch/automation
	MAINTAINER:=luz@plan44.ch
	MENU:=1
endef


define Package/$(PKG_NAME)/description
 contains the configuration setup for P44-BANDIT
endef


# derive P44 platform identifier
ifeq ($(CONFIG_TARGET_brcm2708_bcm2708_RaspberryPi),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=raspberry-pi-1
endif
ifeq ($(CONFIG_TARGET_ar71xx_generic_OMEGA),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=onion-omega
endif
ifeq ($(CONFIG_TARGET_ar71xx_generic_CARAMBOLA2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=carambola2
endif
ifeq ($(CONFIG_TARGET_ramips_mt76x8_DEVICE_omega2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=omega2
endif
ifeq ($(CONFIG_TARGET_ramips_mt7688_DEVICE_omega2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=omega2
endif
ifeq ($(CONFIG_TARGET_ramips_mt7688_DEVICE_P44DSBOmega2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=p44-dsb-omega2
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
	$(CP) $(PKG_BUILD_DIR)/config_p44defaults_${P44_CONFIG_PLATFORM_IDENTIFIER}/* $(1)/etc/config_p44defaults
	$(CP) $(PKG_BUILD_DIR)/config_p44defaults_${P44_CONFIG_PLATFORM_IDENTIFIER}/* $(1)/etc/config
	$(CP) $(PKG_BUILD_DIR)/www/* $(1)/www
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sbin/* $(1)/sbin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/keep-leth $(1)/lib/upgrade/keep.d/leth
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
