# Copyright (c) 2016-2025 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=vdcd
# version of what we are downloading
PKG_VERSION:=2.8.2.21
# version of this makefile
PKG_RELEASE:=252

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=98ef12a663b1d90bd0e4090136b876c991710da8
# Hash can be regenerated with make package/${PKG_NAME}/check FIXUP=1
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/plan44/vdcd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
# package is not mirrored anywhere
PKG_SOURCE_MIRROR:=0

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_LICENSE:=GPL-3.0-or-later
PKG_LICENSE_FILES:=COPYING

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	DEPENDS:=+libstdcpp +boost +libjson-c +libprotobuf-c +libsqlite3 +libopenssl +libavahi-client +librt \
	  +VDCD_OLA:ola \
	  +VDCD_UBUS:libubus +VDCD_UBUS:libblobmsg-json \
	  +VDCD_I2C:libi2c \
	  +VDCD_PNG:libpng \
	  +VDCD_RRDB:librrd1 \
	  +VDCD_EV:libev \
	  +VDCD_UWSC:libuwsc \
	  $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=products
	TITLE:=plan44 digitalstrom virtual device connector daemon
	URL:=https://github.com/plan44/vdcd
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 vdcd is a free (opensource, GPLv3) virtual device connector (vdc) implementation
 for Digital Strom systems. A vdc integrates third-party automation hardware as
 virtual devices into a Digital Strom system.
endef

define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef


CONFIGURE_ARGS += \
	--enable-p44buildow \
	$(if $(CONFIG_DEBUG),--enable-debug) \
	$(call autoconf_bool,CONFIG_VDCD_OLA,ola) \
	$(call autoconf_bool,CONFIG_VDCD_PLATFORM_SHORTBREAK,platformshortbreak) \
	$(call autoconf_bool,CONFIG_VDCD_UBUS,ubus) \
	$(call autoconf_bool,CONFIG_VDCD_I2C,i2c) \
	$(call autoconf_bool,CONFIG_VDCD_PNG,png) \
	$(call autoconf_bool,CONFIG_VDCD_RPIWS281X,rpiws281x) \
	$(call autoconf_bool,CONFIG_VDCD_RRDB,rrdb) \
	$(call autoconf_bool,CONFIG_VDCD_P44FEATURES,p44features) \
	$(call autoconf_bool,CONFIG_VDCD_EV,ev) \
	$(call autoconf_bool,CONFIG_VDCD_UWSC,uwsc) \
	$(call autoconf_bool,CONFIG_VDCD_LVGL,lvgl) \
	$(call autoconf_bool,CONFIG_VDCD_DS485,ds485) \
	$(call autoconf_bool,CONFIG_VDCD_MODBUS,modbus) \
	$(call autoconf_bool,CONFIG_VDCD_EMBEDDED_LIBMODBUS,embeddedlibmodbus) \
	APPLICATION_VERSION=$(PKG_VERSION)


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
