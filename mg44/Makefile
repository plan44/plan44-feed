# Copyright (c) 2016-2024 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=mg44
# version of what we are downloading
PKG_VERSION:=3.13
# version of this makefile
PKG_RELEASE:=20

PKG_SOURCE_PROTO:=git
#PKG_SOURCE_VERSION:=main
PKG_SOURCE_VERSION:=86f246672347e2e40f979a95869d538e6c0cf992
# Hash can be regenerated with make package/${PKG_NAME}/check FIXUP=1
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/plan44/mg44.git
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
  DEPENDS:=+libjson-c +libopenssl +libpthread +librt $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=products
	TITLE:=mg44 minimal mongoose based webserver
	URL:=https://github.com/plan44/mg44
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
  mg44 is a simple webserver based on civetweb (which in turn is a fork of an older mongoose version)
  It has a built-in JSON proxy for accessing TCP socket based JSON APIs and JSON-processing
  command line tools.
endef

CONFIGURE_ARGS += \
	$(if $(CONFIG_DEBUG),--enable-debug) \
	APPLICATION_VERSION=$(PKG_VERSION)

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
