# Copyright (c) 2021 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=kksdcm-config
# version of what we are downloading (or is contained in the package)
PKG_VERSION:=1.1.1
# version of this makefile
PKG_RELEASE:=3

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_CHECK_FORMAT_SECURITY:=0

PKG_CONFIG_DEPENDS := \
	CONFIG_VERSION_NUMBER

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	DEPENDS:=+p44pagekite +uhttpd +kksdcmd
	SECTION:=astrol
	CATEGORY:=astrol
	SUBMENU:=custom
	TITLE:=KKS-DCM modbus controller
	URL:=https://astrol.ch
	MAINTAINER:=info@astrol.ch
	MENU:=1
endef


define Package/$(PKG_NAME)/description
 contains the configuration setup for KKS-DCM
endef


# derive P44 platform identifier
ifeq ($(CONFIG_TARGET_ramips_mt76x8_DEVICE_KKSDCMOmega2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=kksdcm-omega2
endif
ifeq ($(CONFIG_TARGET_ramips_mt76x8_DEVICE_omega2),y)
  P44_CONFIG_PLATFORM_IDENTIFIER:=omega2
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
	$(INSTALL_DIR) $(1)/usr/share/kksdcm
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_DIR) $(1)/lib/upgrade/keep.d
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(CP) $(PKG_BUILD_DIR)/etc/* $(1)/etc
	$(CP) $(PKG_BUILD_DIR)/config_p44defaults_${P44_CONFIG_PLATFORM_IDENTIFIER}/* $(1)/etc/config_p44defaults
	$(CP) $(PKG_BUILD_DIR)/config_p44defaults_${P44_CONFIG_PLATFORM_IDENTIFIER}/* $(1)/etc/config
	$(CP) $(PKG_BUILD_DIR)/resources/* $(1)/usr/share/kksdcm
	$(CP) $(PKG_BUILD_DIR)/www/* $(1)/www
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sbin/* $(1)/sbin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/keep-kksdcm $(1)/lib/upgrade/keep.d/kksdcm
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/ubus-acls/* $(1)/usr/share/rpcd/acl.d
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
