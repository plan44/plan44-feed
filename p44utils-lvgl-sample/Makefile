# Copyright (c) 2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44utils-lvgl-sample
# version of what we are downloading
PKG_VERSION:=0.2
# version of this makefile
PKG_RELEASE:=2

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=master
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/plan44/p44utils-lvgl-sample.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+libstdcpp +boost +libopenssl +libjson-c +librt +libpng $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=samples
	TITLE:=p44utils based tool using LittlevGL for UI
	URL:=https://github.com/plan44/p44utils-lvgl-sample
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 p44utils based tool using LittlevGL for JSON configurable UI
endef

CONFIGURE_ARGS += \
	--enable-p44buildow

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/p44lvglsample $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
