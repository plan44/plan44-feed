# Copyright (c) 2020 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44featured
# version of what we are downloading
PKG_VERSION:=0.1
# version of this makefile
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=p44featured
#PKG_SOURCE_URL:=https://github.com/plan44/p44featured.git
PKG_SOURCE_URL:=ssh://plan44@plan44.ch/home/plan44/sharedgit/p44featured.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+libstdcpp +libpthread +librt +libpng +boost +libjson-c +libopenssl +libsqlite3 +i2c-tools +P44FEATURED_UBUS:libubus +P44FEATURED_UBUS:libblobmsg-json $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=custom
	TITLE:=p44featured - various hardware features/effects for art&exhibition
	URL:=https://github.com/plan44/lethd
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 p44featured is a daemon providing a library of "features" for various applications
 in exhibitions, experiments, art and fun projects.
 Each "feature" usually drives a particular piece of hardware, such as LEDs, solenoids,
 sensors etc. The features are accessible via a common JSON API (and UBUS, on openwrt)
endef

define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef


CONFIGURE_ARGS += \
	--enable-p44buildow \
	$(call autoconf_bool,CONFIG_P44FEATURED_UBUS,ubus)


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
