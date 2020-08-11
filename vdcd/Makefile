# Copyright (c) 2016-2020 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=vdcd
# version of what we are downloading
PKG_VERSION:=2.4.9.0
# version of this makefile
PKG_RELEASE:=115

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=master
PKG_SOURCE_URL:=https://github.com/plan44/vdcd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+libstdcpp +boost +libjson-c +libprotobuf-c +libsqlite3 +libopenssl +i2c-tools +libavahi-client +librt +VDCD_OLA:ola +VDCD_PNG:libpng +VDCD_UBUS:libubus +VDCD_UBUS:libblobmsg-json +VDCD_RRDB:librrd1 $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=products
	TITLE:=plan44 digitalstrom virtual device connector daemon
	URL:=https://github.com/plan44/vdcd
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 vdcd is a free (opensource, GPLv3) virtual device connector (vdc) implementation
 for digitalSTROM systems. A vdc integrates third-party automation hardware as
 virtual devices into a digitalSTROM system.
endef

define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef


CONFIGURE_ARGS += \
	--enable-p44buildow \
	$(call autoconf_bool,CONFIG_VDCD_OLA,ola) \
	$(call autoconf_bool,CONFIG_VDCD_UBUS,ubus) \
	$(call autoconf_bool,CONFIG_VDCD_PNG,png) \
	$(call autoconf_bool,CONFIG_VDCD_RPIWS281X,rpiws281x) \
	$(call autoconf_bool,CONFIG_VDCD_RRDB,rrdb)


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
