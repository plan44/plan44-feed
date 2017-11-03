# Copyright (c) 2017 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=lora_gateway
# version of what we are downloading
PKG_VERSION:=3.1.0a
# version of this makefile
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
# - berkutta (with SPI halfduplex patch)
PKG_SOURCE_URL:=https://github.com/berkutta/lora_gateway.git
PKG_SOURCE_VERSION:=legacy
# - mirakonta (who had a OpenWrt package already)
#PKG_SOURCE_URL:=https://github.com/mirakonta/lora_gateway.git
#PKG_SOURCE_VERSION:=a01a504e004bbe2e6a18c169fb9e2de2f617cb94

# Note: we need to override the standard "packagename-packageversion" here, because
#   packet_forwarder has a ugly ../../lora_gateway reference to the build dir of this package
# Where temporary checkout will be stored, and thus how the .tar.gz will be named (=default PKG_SOURCE)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)
# override build dir, because default is "packagename-packageversion" as well
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)


include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  TITLE:=Semtech lora-gateway package for MT7688 hosts
	DEPENDS:=@(TARGET_ramips_mt76x8||TARGET_ramips_mt7688)
  SECTION:=plan44
  CATEGORY:=plan44
	SUBMENU:=maker
	URL:=https://github.com/plan44/plan44-feed
  URL:=http://www.semtech.com/wireless-rf/lora.html
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
  lora-gateway is a library to build a gateway based on
  a Semtech LoRa multi-channel RF receiver (a.k.a. concentrator).
  Note: currently this package is hardcoded to Mediatek MT76xx,
  and thus dependent on that target. TODO: make it generic
endef

TARGET_CFLAGS += $(FPIC) -O2 -Wall -Wextra -std=c99 -Iinc -I. -lm

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/$(PKG_NAME)
	$(CP) $(PKG_BUILD_DIR)/libloragw/inc/* $(1)/usr/include/$(PKG_NAME)
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libloragw/libloragw.a $(1)/usr/lib/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/root/lora
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libloragw/test* $(1)/root/lora
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_pkt_logger/util_pkt_logger $(1)/root/lora
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_tx_continuous/util_tx_continuous $(1)/root/lora
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_spi_stress/util_spi_stress $(1)/root/lora
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_tx_test/util_tx_test $(1)/root/lora
endef

$(eval $(call HostBuild))
$(eval $(call BuildPackage,$(PKG_NAME)))
