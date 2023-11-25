# Copyright (c) 2020 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=ace-p44script
# version of what we are downloading
PKG_VERSION:=1.30.1
# version of this makefile
PKG_RELEASE:=5

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=master
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/plan44/ace-p44script.git
#PKG_SOURCE_URL:=ssh://plan44@plan44.ch/home/plan44/sharedgit/ace-p44script.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=products
	TITLE:=Ace code editor customized for p44script
	URL:=http://plan44.ch
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 This package contains a size-minimized version of the Ace embeddable source code editor
 (see https://github.com/ajaxorg/ace), customized for p44script editing.
endef

define Build/Compile
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/www
	$(INSTALL_DIR) $(1)/www/ace
	$(CP) $(PKG_BUILD_DIR)/ace/* $(1)/www/ace
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
