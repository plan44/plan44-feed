# Copyright (c) 2016 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

PKG_NAME:=serialfwd
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=ow_testing
PKG_SOURCE_URL:=ssh://plan44@plan44.ch/home/plan44/gitrepositories/serialfwd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

#PKG_CONFIG_DEPENDS:=
#PKG_BUILD_DEPENDS:=

include $(INCLUDE_DIR)/package.mk
$(call include_mk, python-package.mk)

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

# Defaults to just configure - use the following to pass extra args to make
# define Build/Configure
#   $(call Build/Configure/Default,--enable-p44buildow)
# endef

# Defaults to just make - use the following to pass extra args to make
# define Build/Compile
#   $(call Build/Compile/Default,FOO=bar)
# endef

# Installation of compiled result (note this is different from package install)
# Defaults to just make install - use the following to pass extra args to make (need to specify the "install" target!)
# define Build/Install
#   $(call Build/Install/Default,install install-foo)
# endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
