# Copyright (c) 2023 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p-net
# version of what we are downloading
PKG_VERSION:=0.2.0
# version of this makefile
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
#PKG_SOURCE_VERSION:=master
PKG_SOURCE_VERSION:=41ae6e785b7aa91718b4dc9bd5310631f5c4e1c5
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/rtlabs-com/p-net.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1
CMAKE_INSTALL:=1

PKG_BUILD_DEPENDS:=+osal

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
	DEPENDS:=+libpthread +librt $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=custom
	TITLE:=Profinet device stack for embedded systems
	URL:=https://github.com/rtlabs-com/p-net
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
	The RT-Labs Profinet stack p-net is used for Profinet device implementations.
	It is easy to use and provides a small footprint. It is especially well suited
	for embedded systems where resources are limited and efficiency is crucial.
	The stack is supplied with f	ull sources including porting layers and a sample
	application.
endef

define Build/Configure
	$(call Build/Configure/Default)
endef


define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/include/pnal_config.h $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/include/pnet_api.h $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/include/pnet_export.h $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/include/pnet_options.h $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/include/pnet_version.h $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/cmake
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/cmake/ProfinetConfig.cmake $(1)/usr/cmake
	$(CP) $(PKG_BUILD_DIR)/ipkg-install/usr/cmake/ProfinetConfig-release.cmake $(1)/usr/cmake
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ipkg-install/usr/lib/libprofinet.a $(1)/usr/lib/
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ipkg-install/usr/bin/pn_dev $(1)/usr/bin/
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
