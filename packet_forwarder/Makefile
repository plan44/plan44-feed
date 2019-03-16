# Copyright (c) 2017 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=packet_forwarder
# version of this makefile
PKG_RELEASE:=2

PKG_SOURCE_PROTO:=git
# - original
# version of what we are downloading
PKG_VERSION:=2.1.0
PKG_SOURCE_URL:=https://github.com/TheThingsNetwork/packet_forwarder.git
PKG_SOURCE_VERSION:=legacy
# - kersing multi-protocol (MP) - does not compile yet, has path dependency beyond its own source
#PKG_VERSION:=3.0.20
#PKG_SOURCE_URL:=https://github.com/kersing/packet_forwarder.git
#PKG_SOURCE_VERSION:=master


PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)


include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  TITLE:=Semtech packet_forwarder package
	DEPENDS:=+lora_gateway
  SECTION:=plan44
  CATEGORY:=plan44
	SUBMENU:=maker
	URL:=https://github.com/plan44/plan44-feed
  URL:=http://www.semtech.com/wireless-rf/lora.html
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
  LoRa Packet forwarder for Linux based gateways
endef

TARGET_CFLAGS += $(FPIC) -O2 -Wall -Wextra -std=c99 -Iinc -I. -lm

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gps_pkt_fwd/gps_pkt_fwd $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/basic_pkt_fwd/basic_pkt_fwd $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/beacon_pkt_fwd/beacon_pkt_fwd $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
