# Copyright (c) 2017 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=hxcmodplayer
# version of sources
PKG_VERSION:=1.0
# version of this makefile
PKG_RELEASE:=2

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_CHECK_FORMAT_SECURITY:=0

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+alsa-lib $(C_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=sound
	TITLE:=hxcmodplayer MOD music player
	URL:=https://github.com/jfdelnero/HxCModPlayer
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
	hxcmodplayer is a MOD music player with output to ALSA pcm,
	based on the HxCModPlayer engine
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
