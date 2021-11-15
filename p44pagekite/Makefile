# Copyright (c) 2017-2021 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44pagekite
# version of what we are downloading
PKG_VERSION:=1.1
# version of this makefile
PKG_RELEASE:=3

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_CHECK_FORMAT_SECURITY:=0

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	DEPENDS:=+libpagekite +libopenssl $(C_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=Utilities
	TITLE:=plan44 libpagekite based pagekite service
	URL:=https://plan44.ch/automation
	MAINTAINER:=luz@plan44.ch
	MENU:=1
endef


define Package/$(PKG_NAME)/description
 plan44 libpagekite based pagekite service.
 The service is based on a enhanced version of the pagekite tool, p44pagekite,
 which in addition to the standard pagekite tool functionality, can fetch the
 actual kite operating parameters from a list of config files (-f option),
 and adds a mechanism to let kites expire at a specified time to implement
 temporary support connections.
 The service needs runit (part of busybox or separate) installed to run as-is.
 The run script /etc/service/pagekite/run is configured to use the plan44
 pagekite frontent, which is not publicly accessible.
 So this package is primarily intended for plan44 related projects. If you
 want to use it you must modifiy the startup script to use your own
 pagekitefrontend server.
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/etc
	$(CP) ./files/* $(PKG_BUILD_DIR)/
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc
	$(CP) $(PKG_BUILD_DIR)/etc/* $(1)/etc
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
