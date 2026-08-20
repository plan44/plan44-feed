# Copyright (c) 2016-2026 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=vdcd
# version of what we are downloading
PKG_VERSION:=2.8.4.4
# version of this makefile
PKG_RELEASE:=286

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=791c5390159eb63ffced9f64c855c1592d43f86f
# Hash can be regenerated with make package/${PKG_NAME}/check FIXUP=1
PKG_MIRROR_HASH:=skip
PKG_SOURCE_URL:=https://github.com/plan44/vdcd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
# package is not mirrored anywhere
PKG_SOURCE_MIRROR:=0

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_LICENSE:=GPL-3.0-or-later
PKG_LICENSE_FILES:=COPYING

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0
PKG_BUILD_PARALLEL:=1

PKG_BUILD_DEPENDS := protobuf-c/host

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
	DEPENDS:=+libstdcpp +boost +libjson-c +libprotobuf-c +libsqlite3 +libopenssl +libavahi-client +librt \
	  +VDCD_OLA:ola \
	  +VDCD_UBUS:libubus +VDCD_UBUS:libblobmsg-json \
	  +VDCD_I2C:libi2c \
	  +VDCD_PNG:libpng \
	  +VDCD_RRDB:librrd1 \
	  +VDCD_EV:libev \
	  +VDCD_UWSC:libuwsc \
	  +VDCD_LVGLDRM:libdrm \
	  +PACKAGE_libgpiod:libgpiod \
	  $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=products
	TITLE:=plan44 digitalstrom virtual device connector daemon
	URL:=https://github.com/plan44/vdcd
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
 vdcd is a free (opensource, GPLv3) smarthome device connector (vdc) implementation,
 for standalone operation or as companion in a Digital Strom system.
 A vdc integrates third-party automation hardware under a common device model that
 can also be bridged into matter smarthomes via the p44mbrd matter bridge daemon.
endef

define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef


CMAKE_OPTIONS += \
	-DP44_BUILD_OW=ON \
	-DDEBUG=$(if $(CONFIG_DEBUG),ON,OFF) \
	-DENABLE_OLA=$(if $(CONFIG_VDCD_OLA),ON,OFF) \
	-DPLATFORM_HAS_SHORTBREAK=$(if $(CONFIG_VDCD_PLATFORM_SHORTBREAK),ON,OFF) \
	-DENABLE_UBUS=$(if $(CONFIG_VDCD_UBUS),ON,OFF) \
	-DENABLE_I2C=$(if $(CONFIG_VDCD_I2C),ON,OFF) \
	-DENABLE_PNG=$(if $(CONFIG_VDCD_PNG),ON,OFF) \
	-DENABLE_RPIWS281X=$(if $(CONFIG_VDCD_RPIWS281X),ON,OFF) \
	-DENABLE_RRDB=$(if $(CONFIG_VDCD_RRDB),ON,OFF) \
	-DENABLE_P44FEATURES=$(if $(CONFIG_VDCD_P44FEATURES),ON,OFF) \
	-DENABLE_EV=$(if $(CONFIG_VDCD_EV),ON,OFF) \
	-DENABLE_UWSC=$(if $(CONFIG_VDCD_UWSC),ON,OFF) \
	-DENABLE_GPIOD=$(if $(CONFIG_PACKAGE_libgpiod),ON,OFF) \
	-DENABLE_LVGL=$(if $(CONFIG_VDCD_LVGL),ON,OFF) \
	-DENABLE_LVGL_DRM=$(if $(CONFIG_VDCD_LVGLDRM),ON,OFF) \
	-DENABLE_DS485=$(if $(CONFIG_VDCD_DS485),ON,OFF) \
	-DENABLE_MCTRL=$(if $(CONFIG_VDCD_MCTRL),ON,OFF) \
	-DENABLE_WBF=$(if $(CONFIG_VDCD_WBF),ON,OFF) \
	-DENABLE_MODBUS=$(if $(CONFIG_VDCD_MODBUS),ON,OFF) \
	-DEMBEDDED_LIBMODBUS=$(if $(CONFIG_VDCD_EMBEDDED_LIBMODBUS),ON,OFF) \
	-DLVGL_COLOR_DEPTH="$(CONFIG_VDCD_LVGLCOLORDEPTH)" \
	-DP44_EXPECTED_APP_VERSION="$(PKG_VERSION)"

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
