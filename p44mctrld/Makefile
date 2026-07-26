# Copyright (c) 2022-2026 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44mctrld
# version of what we are downloading
PKG_VERSION:=0.1.3
# version of this makefile
PKG_RELEASE:=3

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=e14b7690dd17b2eeeb96d57d54595b3cb3d26405
# Hash can be regenerated with make package/${PKG_NAME}/check FIXUP=1
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/plan44/p44mctrld.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
# package is not mirrored anywhere
PKG_SOURCE_MIRROR:=0


PKG_LICENSE:=GPL-3.0-or-later
PKG_LICENSE_FILES:=COPYING

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_CHECK_FORMAT_SECURITY:=0
PKG_BUILD_PARALLEL:=1
PKG_BUILD_DEPENDS:=gn/host glib2/host

# Development builder setup: when the local checkout is mounted into the
# dockerized OpenWrt tree, use it as the package source without affecting
# dependencies. Normal builds keep using PKG_SOURCE_URL.
P44MCTRLD_SOURCE_DIR ?= /work/p44mctrld
ifneq ($(wildcard $(P44MCTRLD_SOURCE_DIR)/src/p44mctrld_main.cpp),)
USE_SOURCE_DIR:=$(P44MCTRLD_SOURCE_DIR)
endif

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
	DEPENDS:=+libstdcpp +boost +libjson-c +libopenssl +libavahi-client +librt +libev +libatomic +glib2 +dbus $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=products
	TITLE:=p44mctrld - matter controller daemon for use with vdcd
	URL:=https://plan44.ch/matter
	MAINTAINER:=luz@plan44.ch
	MENU:=1
endef


define Package/$(PKG_NAME)/description
 matter bridge daemon suitable for running as companion to vdcd smart home device controller
endef


# https://github.com/project-chip/matter-openwrt/issues/2
ifeq ($(DUMP)$(filter SUBMODULES:=,$(Download/Defaults)),)
$(error PKG_SOURCE_SUBMODULES is not supported, ensure https://github.com/openwrt/openwrt/pull/13000 is included in your OpenWrt buildroot)
endif

# https://github.com/openwrt/openwrt/issues/13016
TARGET_CXXFLAGS += -Wno-format-nonliteral -DP44_BUILD_OW=1

# The build environment contains host tools that can be shared between targets
CHIP_BUILD_ENV_DIR:=$(STAGING_DIR_HOST)/share/chip-build-env
OUT_DIR:=$(PKG_BUILD_DIR)/out/openwrt

# define Build/Prepare
# 	$(call Build/Prepare/Default)
# 	mkdir -p $(PKG_BUILD_DIR)/files
# 	$(CP) ./files/* $(PKG_BUILD_DIR)/files
# endef

# Note: --project must be specified relative to CHIP_ROOT
define Build/Configure
	mkdir -p $(OUT_DIR) && cd $(OUT_DIR) && \
		$(CONFIGURE_VARS) $(PKG_BUILD_DIR)/src/third_party/connectedhomeip/scripts/configure \
			--build-env-dir="$(CHIP_BUILD_ENV_DIR)" \
			--project=../.. \
			--target=$(GNU_TARGET_NAME) \
			--enable-wifi=no \
			--enable-openthread=no \
			--enable-tracing-support=no \
			--p44-application-version="$(PKG_VERSION)" \
			--p44-debug-attr-access=0
endef

define Build/Compile
	$(NINJA) -C $(OUT_DIR) ${PKG_NAME}
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/usr/share/p44mctrld
	$(CP) ./files/etc/* $(1)/etc
	#$(CP) ./files/resources/* $(1)/usr/share/p44mctrld
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(OUT_DIR)/${PKG_NAME} $(1)/usr/bin
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
