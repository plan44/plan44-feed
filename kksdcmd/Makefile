# Copyright (c) 2021 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=kksdcmd
# version of what we are downloading
PKG_VERSION:=1.2.0
# version of this makefile
PKG_RELEASE:=6

PKG_SECONDARY_NAME:=p44mbutil

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=main
PKG_MIRROR_HASH:=skip
#PKG_SOURCE_URL:=ssh://plan44@plan44.ch/home/plan44/sharedgit/kksdcmd.git
PKG_SOURCE_URL:=https://github.com/plan44/kksdcmd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/Default
  DEPENDS:=+libstdcpp  +libpthread +librt +boost +libjson-c +libsqlite3 +libopenssl +libpng +libi2c +KKSDCMD_UBUS:libubus +KKSDCMD_UBUS:libblobmsg-json +!KKSDCMD_EMBEDDED_LIBMODBUS:libmodbus $(CXX_DEPENDS)
	SECTION:=astrol
	CATEGORY:=astrol
	SUBMENU:=custom
	MAINTAINER:=info@astrol.ch
endef


define Package/$(PKG_NAME)
  $(call Package/$(PKG_NAME)/Default)
	TITLE:=daemon for modbus slave/master with web interface
endef

define Package/$(PKG_NAME)/description
 daemon for modbus slave/master with web interface
endef

define Package/$(PKG_SECONDARY_NAME)
  $(call Package/$(PKG_NAME)/Default)
	TITLE:=modbus command line utility
endef

define Package/$(PKG_SECONDARY_NAME)/description
 modbus command line utility for communicating with generic modbus slaves
endef


define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef


CONFIGURE_ARGS += \
	--enable-p44buildow \
	$(call autoconf_bool,CONFIG_KKSDCMD_UBUS,ubus) \
	$(call autoconf_bool,CONFIG_KKSDCMD_EMBEDDED_LIBMODBUS,embeddedlibmodbus)


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef


define Package/$(PKG_SECONDARY_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/p44mbutil $(1)/usr/bin/
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_SECONDARY_NAME)))
