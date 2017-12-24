# Copyright (c) 2016 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=pixelboardd
# version of what we are downloading
PKG_VERSION:=0.13
# version of this makefile
PKG_RELEASE:=13

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=master
PKG_SOURCE_URL:=https://github.com/plan44/pixelboardd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+libstdcpp +libpthread +librt +libpng +boost +libjson-c +libopenssl +libsqlite3 +i2c-tools $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=toys
	TITLE:=pixelboard daemon
	URL:=https://github.com/plan44/pixelboardd
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 pixelboard is a free (opensource, GPLv3) daemon for for running games and more
 on WS2813 based pixelboard (lounge table) - which is an open source hardware
 project by itself.
endef

define Build/Configure
  $(call Build/Configure/Default,--enable-p44buildow)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
