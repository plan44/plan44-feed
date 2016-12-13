# Copyright (c) 2016 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=vdsm
# version of what we are downloading
PKG_VERSION:=0.1
# version of this makefile
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=ow_testing
PKG_SOURCE_URL:=ssh://plan44@plan44.ch/home/plan44/sharedgit/vdsm.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+libprotobuf-c +libsqlite3 +libopenssl +librt
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=digitalstrom
	TITLE:=vdSM - virtual digitalstrom meter
	URL:=https://git.digitalstrom.org/virtual-devices/vdsm
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 vdSM is a component needed on external platforms as a temporary solution until
 dSS finally provides is as a system component on all productive digitalSTROM servers.
endef

define Build/Configure
  $(call Build/Configure/Default,--enable-p44buildow)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/vdSM $(1)/usr/bin/$(PKG_NAME)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
