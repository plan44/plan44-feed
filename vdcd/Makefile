# Copyright (c) 2016 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=vdcd
# version of what we are downloading
PKG_VERSION:=0.1
# version of this makefile
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=ow_testing
PKG_SOURCE_URL:=ssh://plan44@plan44.ch/home/plan44/sharedgit/vdcd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+boost +libjson-c +libprotobuf-c +libsqlite3 +libopenssl $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=digitalstrom
	TITLE:=plan44 digitalstrom virtual device connector daemon
	URL:=https://github.com/plan44/vdcd
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 vdcd is a free (opensource, GPLv3) virtual device connector (vdc) implementation
 for digitalSTROM systems. A vdc integrates third-party automation hardware as
 virtual devices into a digitalSTROM system.
endef

define Build/Configure
  $(call Build/Configure/Default,--enable-p44buildow)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
