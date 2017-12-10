# Copyright (c) 2017 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=libxm
# version of what we are downloading
PKG_VERSION:=0.2
# version of this makefile
PKG_RELEASE:=1

PKG_DOWNLOAD_NAME:=v$(PKG_VERSION)
PKG_SOURCE:=$(PKG_DOWNLOAD_NAME).tar.gz
PKG_SOURCE_URL:=https://github.com/Artefact2/libxm/archive/
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_HASH:=2b3268cdabefd006bc45fe3c59149ff3a81d434708461bbda0b689bae9c4c724
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+alsa-lib $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=sound
	TITLE:=libxm extended mod (xm) player libary and tools
	URL:=https://github.com/Artefact2/libxm
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 libxm extended mod (xm) player libary and tools
endef

TARGET_CPPFLAGS += -I$(LINUX_DIR)/include

define Build/Configure
  $(call Build/Configure/Default,--without-x --enable-audio=alsa)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/examples/xmltoasla $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libxm.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))

