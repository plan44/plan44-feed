include $(TOPDIR)/rules.mk
#Name and release number of this package
PKG_NAME:=sshpass
PKG_RELEASE:=1.5

PKG_BUILD_DIR :=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/sshpass
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=sshpass
endef

define Package/sshpass/description
	Sshpass is a tool for non-interactivly performing password authentication with SSH's
endef

#Specify what needs to be done to prepare for building the package.
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

#Specify where and how to install the program.
define Package/sshpass/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sshpass $(1)/bin/
endef

#This line executes the necessary commands to compile our program.
$(eval $(call BuildPackage,sshpass))
