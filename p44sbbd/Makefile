# Copyright (c) 2016 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44sbbd
# version of what we are downloading
PKG_VERSION:=0.1
# version of this makefile
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=newmodules
PKG_SOURCE_URL:=ssh://plan44@plan44.ch/home/plan44/sharedgit/p44sbbd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+libstdcpp +boost +libjson-c +libsqlite3 +librt $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=toys
	TITLE:=daemon for controlling Swiss Railway (SBB) displays
	URL:=https://github.com/plan44/p44sbbd
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 p44sbbd is a free (opensource, GPLv3) daemon for controlling Swiss Railway (SBB)
 electromechanical display modules (Fallblattanzeigen) via a RS485 serial interface.
 It supports USB to RS485 converters as well as directly wired RS485 level shifters
 using GPIOs to control line drivers.
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/service
	$(CP) $(PKG_BUILD_DIR)/etc/service/* $(1)/etc/service
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
