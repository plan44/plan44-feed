# Copyright (c) 2016-2024 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=serialfwd
# version of what we are downloading
PKG_VERSION:=1.3
# version of this makefile
PKG_RELEASE:=5

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=76dd1384780d867014fef32d71b5e233c37f7d70
# Hash can be regenerated with make package/${PKG_NAME}/check FIXUP=1
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/plan44/serialfwd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=Utilities
	TITLE:=serial protocol testing and forwarding tool
	URL:=http://plan44.ch/opensource.php
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 This package provides the serialfwd tool, which helps testing/debugging serial protocols
 on the command line by allowing to send/receive bytes specified in hex
 and can also act as a proxy to access a serial connection on a (embedded) target host
 via TCP/IP from a remote (development) host.
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
