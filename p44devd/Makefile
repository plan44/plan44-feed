# Copyright (c) 2019 Lukas Zeller <luz@plan44.ch>
#
# This is free software, licensed under the GNU General Public License v2.
# See COPYING for more information.
#

include $(TOPDIR)/rules.mk

# name
PKG_NAME:=p44devd
# version of what we are downloading
PKG_VERSION:=0.1
# version of this makefile
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_CHECK_FORMAT_SECURITY:=0

PKG_FIXUP:=autoreconf

# MIPS16 support leads to strange "{standard input}: Assembler messages:", so we turn it off (not needed anyway)
PKG_USE_MIPS16:=0

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  DEPENDS:=+libubus +libblobmsg-json $(C_DEPENDS)
	SECTION:=plan44
	CATEGORY:=plan44
	SUBMENU:=products
	TITLE:=plan44 platform device management ubus server daemon
	MAINTAINER:=luz@plan44.ch
endef

define Package/$(PKG_NAME)/description
  The daemon provides some ubus objects which can be called from uhttpd based web-UIs
  as well as from service scripts. The objects provide p44 specific services in
  a platform abstracted way.
endef


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(PKG_NAME) $(1)/sbin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/$(PKG_NAME).init $(1)/etc/init.d/$(PKG_NAME)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
