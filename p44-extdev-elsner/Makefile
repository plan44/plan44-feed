# Copyright (c) 2018-2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44-extdev-elsner
# version of what we are downloading
PKG_VERSION:=2.0
PKG_MIRROR_HASH:=skip
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
	TITLE:=p44 external device for Elsner P03/P04 weather station
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
  p44 external device for Elsner P03/P04 weather station - installable on P44-DSB devices
endef


P44UTILS_PROTO:=git
P44UTILS_URL:=https://github.com/plan44/p44utils.git
P44UTILS_VERSION:=aaf0f22468c0c98f50775128cb8a5b45ee183b7f
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
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/p44extdevelsner $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/files/etc/* $(1)/etc
	$(CP) $(PKG_BUILD_DIR)/files/www/* $(1)/www
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
