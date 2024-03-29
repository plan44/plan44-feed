# Copyright (c) 2017-2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44banditd
# version of what we are downloading
PKG_VERSION:=1.1
# version of this makefile
PKG_RELEASE:=7

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=master
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/plan44/p44banditd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+libstdcpp +libpthread +librt +boost +libjson-c +libopenssl +libsqlite3 +i2c-tools $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=custom
	TITLE:=p44bandit daemon
	URL:=https://github.com/plan44/pixelboardd
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 p44banditd is a free (opensource, GPLv3) daemon for for interfacing with
 ancient BANDIT CNC mill controller
endef

define Build/Configure
  $(call Build/Configure/Default,--enable-p44buildow)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
