# Copyright (c) 2020 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44-extdev-p44motorexp
# version of what we are downloading
PKG_VERSION:=1.0
# version of this makefile
PKG_RELEASE:=5

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_CHECK_FORMAT_SECURITY:=0

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+libstdcpp +librt +boost +libjson-c $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=products
	TITLE:=p44 external device for P44 DC motor expansion
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
  p44 external device for Elsner P44 DC motor controller expansion board (for P44-DSB-E2 and P44-LC-E base hardware)
endef


P44UTILS_PROTO:=git
P44UTILS_URL:=https://github.com/plan44/p44utils.git
P44UTILS_VERSION:=dcmotor
P44UTILS_FILE:=p44utils-$(P44UTILS_VERSION).gz
P44UTILS_SUBDIR:=p44utils-$(P44UTILS_VERSION)
define Download/p44utils
  FILE:=$(P44UTILS_FILE)
  SUBDIR:=$(P44UTILS_SUBDIR)
  PROTO:=$(P44UTILS_PROTO)
  URL:=$(P44UTILS_URL)
  VERSION:=$(P44UTILS_VERSION)
endef
$(eval $(call Download,p44utils))


define Build/Prepare
  $(Build/Prepare/Default)
	mkdir -p $(PKG_BUILD_DIR)/files
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	$(CP) ./files/* $(PKG_BUILD_DIR)/files
	# prepare p44utils
	(cd $(PKG_BUILD_DIR)/src && tar -xzf "$(DL_DIR)/$(P44UTILS_FILE)" && mv $(P44UTILS_SUBDIR) p44utils)
endef

define Build/Configure
  $(call Build/Configure/Default)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/www
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/p44extdevmotorexp $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/files/etc/* $(1)/etc
	$(CP) $(PKG_BUILD_DIR)/files/www/* $(1)/www
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
