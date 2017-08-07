# Copyright (c) 2017 plan44.ch / Lukas Zeller, Zurich, Switzerland
#
# Author: Lukas Zeller <luz@plan44.ch>

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=timidity
# version of what we are downloading
PKG_VERSION:=2.13.0
# version of this makefile
PKG_RELEASE:=1

PKG_SOURCE:=TiMidity++-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/timidity
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_MD5SUM:=a006fad0432b12772d5e0937f0bb51ba
PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=COPYING

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
#  DEPENDS:=+this +that $(CXX_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=sound
	TITLE:=timidity MIDI software synthesizer
	URL:=http://timidity.sourceforge.net
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
  TiMidity++ is a software synthesizer. It can play MIDI files by converting them into
  PCM waveform data; give it a MIDI data along with digital instrument data files,
  then it synthesizes them in real-time, and plays. It can not only play sounds,
  but also can save the generated waveforms into hard disks as various audio file formats.

  TiMidity++ is a free software, distributed under the terms of GNU general public license.
endef

define Build/Configure
  $(call Build/Configure/Default,--without-x --enable-audio=alsa)
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
