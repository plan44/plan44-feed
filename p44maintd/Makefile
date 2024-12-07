# Copyright (c) 2024 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44maintd
# version of what we are downloading
PKG_VERSION:=4.5.1
# version of this makefile
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=530f722e1c9f5297054815aa6f91ab46427c3e5f
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/plan44/p44maintd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
# package is not mirrored anywhere
PKG_SOURCE_MIRROR:=0

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	DEPENDS:=+libstdcpp  +libpthread +librt +boost +libjson-c $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=Utilities
	TITLE:=plan44 platform configuration utility
	URL:=https://plan44.ch/automation
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 plan44 platform configuration utility
endef

CONFIGURE_ARGS += \
	--enable-p44buildow \
	$(if $(CONFIG_DEBUG),--enable-debug) \
	APPLICATION_VERSION=$(PKG_VERSION)

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
