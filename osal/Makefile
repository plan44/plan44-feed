# Copyright (c) 2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=osal
# version of what we are downloading
PKG_VERSION:=0.1.0
# version of this makefile
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
#PKG_SOURCE_VERSION:=master
PKG_SOURCE_VERSION:=552713d5dafb28554a30633e057e82fc345eafc3
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/rtlabs-com/osal.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1
CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
	DEPENDS:=+libpthread +librt
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=custom
	TITLE:=rtlabs.com OS abstraction layer
	URL:=https://github.com/rtlabs-com/osal
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
	The goal of the abstraction layer is to simplify writing software that
	runs on many platforms. This abstraction layer is mainly focused on
	threading functionality.
endef

define Build/Configure
	$(call Build/Configure/Default)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/include/osal.h $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/include/osal_log.h $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/include/sys
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/include/sys/osal_cc.h $(1)/usr/include/sys
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/include/sys/osal_sys.h $(1)/usr/include/sys
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ipkg-install/usr/lib/libosal.a $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/cmake
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/cmake/OsalTargets.cmake $(1)/usr/cmake
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/cmake/OsalTargets-release.cmake $(1)/usr/cmake
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/cmake/OsalConfig.cmake $(1)/usr/cmake
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ipkg-install/usr/bin/pn_dev $(1)/usr/bin/
endef


define Package/$(PKG_NAME)/install
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
