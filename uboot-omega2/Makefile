# SPDX-License-Identifier: GPL-2.0-only

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=uboot-omega2
PKG_VERSION:=4.3.0.3
PKG_RELEASE:=$(AUTORELEASE)

#PKG_HASH:=fe732aaf037d9cc3c0909bad8362af366ae964bbdac6913a34081ff4ad565372

# override built-in standard uboot source, use onion's version instead
PKG_SOURCE_PROTO = 1 # prevents u-boot.mk to use standard sources

# from u-boot.mk:
#
#PKG_SOURCE_URL = \
#	https://mirror.cyberbits.eu/u-boot \
#	https://ftp.denx.de/pub/u-boot \
#	ftp://ftp.denx.de/pub/u-boot

PKG_SOURCE_PROTO:=git
#PKG_SOURCE_URL:=https://github.com/OnionIoT/omega2-bootloader
#PKG_SOURCE_VERSION:=aa4d13314f6e2553d8aaf871acb6bd2643b32199
PKG_SOURCE_URL:=https://github.com/plan44/omega2-bootloader
PKG_SOURCE_VERSION:=main
PKG_MIRROR_HASH:=skip
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
# package is not mirrored anywhere
PKG_SOURCE_MIRROR:=0

include $(INCLUDE_DIR)/u-boot.mk

include $(INCLUDE_DIR)/package.mk

define U-Boot/Default
  BUILD_TARGET:=ramips
  UBOOT_IMAGE:=u-boot.bin
endef



## CONFIG_TARGET_ramips_mt76x8_DEVICE_onion_omega2 is not set
## CONFIG_TARGET_ramips_mt76x8_DEVICE_onion_omega2p is not set
#CONFIG_TARGET_ramips_mt76x8_DEVICE_plan44_p44dsbomega2=y
#CONFIG_TARGET_ramips_mt76x8_DEVICE_ravpower_rp-wd009


define U-Boot/plan44_p44dsbomega2
  BUILD_DEVICES:=plan44_p44dsbomega2
  BUILD_SUBTARGET:=mt76x8
  NAME:=plan44 p44dsbomega2
  UBOOT_CONFIG:=rt2880
endef

UBOOT_TARGETS := \
	plan44_p44dsbomega2

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/$(UBOOT_IMAGE) $(STAGING_DIR_IMAGE)/$(VARIANT)-$(UBOOT_IMAGE)
endef

$(eval $(call BuildPackage/U-Boot))
