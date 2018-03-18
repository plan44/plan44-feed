#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/cmake.mk

PKG_NAME:=rpi-userland
PKG_VERSION:=20141122
PKG_RELEASE:=1

PKG_REV:=4333d6d023a1f0dcc763d0706a1fe6523f9b6005
PKG_MD5SUM:=3cac9d31589cc7d41e8c03d02de76732

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=${PKG_REV}
PKG_SOURCE_URL:=https://github.com/raspberrypi/userland.git

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_BUILD_DEPENDS:=libfreetype zlib

CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/rpi-userland/Default
  SECTION:=rpi
  CATEGORY:=Raspberry Pi
  URL:=https://github.com/raspberrypi/userland
endef

define Package/rpi-userland
  $(call Package/rpi-userland/Default)
  DEPENDS:=+libpthread +librt @TARGET_brcm2708
  TITLE:=rpi-userland
  MENU:=1
endef

define Package/rpi-userland/description
 Raspberry PI userland libraries for interfaces EGL, mmal, GLESv2,
 vcos, openmaxil, vchiq_arm, bcm_host, WCF and OpenVG
endef

define Package/rpi-vcgencmd
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=vcgencmd
endef

define Package/rpi-raspicam
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=raspicam
endef

define Package/rpi-tvservice
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=tvservice
endef

define Package/rpi-helloaudio
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-helloaudio demo
endef

define Package/rpi-hellodispmanx
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-hellodispmanx demo
endef

define Package/rpi-helloencode
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-helloencode demo
endef

define Package/rpi-hellofont
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland +libfreetype +zlib
  TITLE:=rpi-hellofont demo
endef

define Package/rpi-hellojpeg
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-hellojpeg demo
endef

define Package/rpi-helloteapot
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-helloteapot demo
endef

define Package/rpi-hellotiger
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-hellotiger demo
endef

define Package/rpi-hellotriangle
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-hellotriangle demo
endef

define Package/rpi-hellotriangle2
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-hellotriangle2 demo
endef

define Package/rpi-hellovideo
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-hellovideo demo
endef

define Package/rpi-hellovideocube
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland rpi-hellovideo
  TITLE:=rpi-hellovideocube demo
endef

define Package/rpi-helloworld
  $(call Package/rpi-userland/Default)
  DEPENDS:=rpi-userland
  TITLE:=rpi-helloworld demo
endef

HELLOPI_CFLAGS = -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC \
		-D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT \
		-ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi

HELLOPI_INCLUDES = -I$(PKG_BUILD_DIR)/interface/vcos/pthreads -I$(PKG_BUILD_DIR)/interface/vmcs_host/linux \
		-I$(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/libs/ilclient \
		-I$(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/libs/vgfont \
		-I$(PKG_BUILD_DIR)/interface/vmcs_host/khronos \
		-I$(PKG_BUILD_DIR)/interface/khronos/include \
		-I$(PKG_BUILD_DIR)/host_applications/linux/libs/bcm_host/include \
		-I$(PKG_BUILD_DIR) -I$(STAGING_DIR)/usr/include/freetype2

HELLOPI_LDFLAGS = -L$(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/libs/ilclient \
		-L$(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/libs/vgfont \
		-L$(PKG_BUILD_DIR)/build/lib \
		-lGLESv2 -lEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt

define Build/Prepare
	$(call Build/Prepare/Default)
	mkdir -p $(PKG_BUILD_DIR)/build/arm-linux/release
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR)/build/arm-linux/release/; \
	LDFLAGS="$(TARGET_LDFLAGS) $(EXTRA_LDFLAGS)" \
	cmake \
		-DCMAKE_TOOLCHAIN_FILE=$(TOPDIR)/$(SOURCE)/files/arm-linux-openwrt.cmake \
		-DCMAKE_SYSTEM_VERSION=1 \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER_ARG1="$(CMAKE_C_COMPILER_ARG1)" \
		-DCMAKE_CXX_COMPILER_ARG1="$(CMAKE_CXX_COMPILER_ARG1)" \
		-DCMAKE_EXE_LINKER_FLAGS:STRING="$(TARGET_LDFLAGS)" \
		-DCMAKE_MODULE_LINKER_FLAGS:STRING="$(TARGET_LDFLAGS)" \
		-DCMAKE_SHARED_LINKER_FLAGS:STRING="$(TARGET_LDFLAGS)" \
		-DCMAKE_FIND_ROOT_PATH="$(STAGING_DIR);$(TOOLCHAIN_DIR)" \
		-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH \
		-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
		-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
		-DCMAKE_STRIP=: \
		-DCMAKE_INSTALL_PREFIX=/usr \
		-DDL_LIBRARY=$(STAGING_DIR) \
		-DCMAKE_PREFIX_PATH=$(STAGING_DIR) \
		$(CMAKE_OPTIONS) \
		../../.. \
	)
endef

define Build/Compile
	$(MAKE) -j 6 -C $(PKG_BUILD_DIR)/build/arm-linux/release

	#Compile ilclient support library for some demos
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/libs/ilclient; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c ilclient.c -o ilclient.o -Wno-deprecated-declarations; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c ilcore.c -o ilcore.o -Wno-deprecated-declarations; \
		$(TARGET_AR) r libilclient.a ilclient.o ilcore.o; \
	)

	#Compile vgfont support library for some demos
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/libs/vgfont; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) -D_HAVE_TIMER_T $(HELLOPI_INCLUDES) -g -c font.c -o font.o -Wno-deprecated-declarations; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) -D_HAVE_TIMER_T $(HELLOPI_INCLUDES) -g -c vgft.c -o vgft.o -Wno-deprecated-declarations; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) -D_HAVE_TIMER_T $(HELLOPI_INCLUDES) -g -c graphics.c -o graphics.o -Wno-deprecated-declarations; \
		$(TARGET_AR) r libvgfont.a font.o vgft.o graphics.o; \
	)

	#Build helloaudio demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_audio; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c audio.c -o audio.o -Wno-deprecated-declarations; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c sinewave.c -o sinewave.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_audio -Wl,--whole-archive audio.o sinewave.o $(HELLOPI_LDFLAGS) -lilclient -Wl,--no-whole-archive -rdynamic; \
	)

	#Build hellodispmanx demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_dispmanx; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c dispmanx.c -o dispmanx.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_dispmanx -Wl,--whole-archive dispmanx.o $(HELLOPI_LDFLAGS) -Wl,--no-whole-archive -rdynamic; \
	)

	#Build helloencode demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_encode; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c encode.c -o encode.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_encode -Wl,--whole-archive encode.o $(HELLOPI_LDFLAGS) -lilclient -Wl,--no-whole-archive -rdynamic; \
	)

	#Build hellofont demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_font; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c main.c -o main.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_font -Wl,--whole-archive main.o $(HELLOPI_LDFLAGS) -lvgfont -lfreetype -lz -Wl,--no-whole-archive -rdynamic; \
	)

	#Build hellojpeg demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_jpeg; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c jpeg.c -o jpeg.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_jpeg -Wl,--whole-archive jpeg.o $(HELLOPI_LDFLAGS) -lilclient -Wl,--no-whole-archive -rdynamic; \
	)

	#Build helloteapot demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_teapot; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c triangle.c -o triangle.o -Wno-deprecated-declarations; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c video.c -o video.o -Wno-deprecated-declarations; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c models.c -o models.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_teapot -Wl,--whole-archive triangle.o video.o models.o $(HELLOPI_LDFLAGS) -lm -lilclient -Wl,--no-whole-archive -rdynamic; \
	)

	#Build hellotiger demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_tiger; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) -D__RASPBERRYPI__ $(HELLOPI_INCLUDES) -g -c main.c -o main.o -Wno-deprecated-declarations; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) -D__RASPBERRYPI__ $(HELLOPI_INCLUDES) -g -c tiger.c -o tiger.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_tiger -Wl,--whole-archive main.o tiger.o $(HELLOPI_LDFLAGS) -Wl,--no-whole-archive -rdynamic; \
	)

	#Build hellotriangle demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_triangle; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c triangle.c -o triangle.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_triangle -Wl,--whole-archive triangle.o $(HELLOPI_LDFLAGS) -Wl,--no-whole-archive -rdynamic; \
	)

	#Build hellotriangle2 demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_triangle2; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c triangle2.c -o triangle2.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_triangle2 -Wl,--whole-archive triangle2.o $(HELLOPI_LDFLAGS) -Wl,--no-whole-archive -rdynamic; \
	)

	#Build hellovideo demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_video; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c video.c -o video.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_video -Wl,--whole-archive video.o $(HELLOPI_LDFLAGS) -lilclient -Wl,--no-whole-archive -rdynamic; \
	)

	#Build hellovideocube demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_videocube; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c triangle.c -o triangle.o -Wno-deprecated-declarations; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c video.c -o video.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_videocube -Wl,--whole-archive triangle.o video.o $(HELLOPI_LDFLAGS) -lilclient -Wl,--no-whole-archive -rdynamic; \
	)

	#Build helloworld demo
	(cd $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_world; \
		$(TARGET_CC) $(TARGET_CFLAGS) $(HELLOPI_CFLAGS) $(HELLOPI_INCLUDES) -g -c world.c -o world.o -Wno-deprecated-declarations; \
		$(TARGET_CC) -o hello_world -Wl,--whole-archive world.o $(HELLOPI_LDFLAGS) -Wl,--no-whole-archive -rdynamic; \
	)

endef

define Build/Install
	$(MAKE_VARS) $(MAKE) -C $(PKG_BUILD_DIR)/build/arm-linux/release \
		$(MAKE_INSTALL_FLAGS) install
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib \
		$(1)/usr/include \
		$(1)/usr/src/hello_pi \
		$(1)/usr/lib/pkgconfig

	$(CP) $(PKG_INSTALL_DIR)/opt/vc/lib/*.so* $(1)/usr/lib
	$(CP) -R $(PKG_INSTALL_DIR)/opt/vc/include/** $(1)/usr/include/
	$(CP) -R $(PKG_INSTALL_DIR)/opt/vc/src/hello_pi/** $(1)/usr/src/hello_pi/

	$(INSTALL_DATA) ./files/bcm_host.pc $(1)/usr/lib/pkgconfig/
	$(INSTALL_DATA) ./files/egl.pc $(1)/usr/lib/pkgconfig/
	$(INSTALL_DATA) ./files/glesv2.pc $(1)/usr/lib/pkgconfig/

endef

define Package/rpi-userland/install
	$(INSTALL_DIR) $(1)/usr/lib \
		$(1)/usr/bin \
		$(1)/usr/sbin \
		$(1)/usr/share/install
	$(CP) $(PKG_INSTALL_DIR)/opt/vc/lib/*.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/opt/vc/bin/* $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/opt/vc/sbin/vcfiled $(1)/usr/sbin/
	$(CP) $(PKG_INSTALL_DIR)/opt/vc/share/install/vcfiled $(1)/usr/share/install/
endef

define Package/rpi-vcgencmd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/build/bin/vcgencmd $(1)/usr/bin
endef

define Package/rpi-raspicam/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/build/bin/raspistill $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/build/bin/raspiyuv $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/build/bin/raspivid $(1)/usr/bin
endef

define Package/rpi-tvservice/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/build/bin/tvservice $(1)/usr/bin
endef

define Package/rpi-helloaudio/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_audio/hello_audio $(1)/usr/share/rpi_demos
endef

define Package/rpi-hellodispmanx/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_dispmanx/hello_dispmanx $(1)/usr/share/rpi_demos/
endef

define Package/rpi-helloencode/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_encode/hello_encode $(1)/usr/share/rpi_demos/
endef

define Package/rpi-hellofont/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_font/hello_font $(1)/usr/share/rpi_demos/
endef

define Package/rpi-hellojpeg/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_jpeg/hello_jpeg $(1)/usr/share/rpi_demos/
endef

define Package/rpi-helloteapot/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_teapot/hello_teapot $(1)/usr/share/rpi_demos/
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_teapot/teapot.obj.dat $(1)/usr/share/rpi_demos/
endef

define Package/rpi-hellotiger/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos/
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_tiger/hello_tiger $(1)/usr/share/rpi_demos/
endef

define Package/rpi-hellotriangle/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_triangle/hello_triangle $(1)/usr/share/rpi_demos/
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_triangle/*.raw $(1)/usr/share/rpi_demos/
endef

define Package/rpi-hellotriangle2/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_triangle2/hello_triangle2 $(1)/usr/share/rpi_demos/
endef

define Package/rpi-hellovideo/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_video/hello_video $(1)/usr/share/rpi_demos/
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_video/test.h264 $(1)/usr/share/rpi_demos/
endef

define Package/rpi-hellovideocube/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_videocube/hello_videocube $(1)/usr/share/rpi_demos/
endef

define Package/rpi-helloworld/install
	$(INSTALL_DIR) $(1)/usr/share/rpi_demos
	$(CP) $(PKG_BUILD_DIR)/host_applications/linux/apps/hello_pi/hello_world/hello_world $(1)/usr/share/rpi_demos/
endef

$(eval $(call BuildPackage,rpi-userland))
$(eval $(call BuildPackage,rpi-vcgencmd))
$(eval $(call BuildPackage,rpi-raspicam))
$(eval $(call BuildPackage,rpi-tvservice))
$(eval $(call BuildPackage,rpi-helloaudio))
$(eval $(call BuildPackage,rpi-hellodispmanx))
$(eval $(call BuildPackage,rpi-helloencode))
$(eval $(call BuildPackage,rpi-hellofont))
$(eval $(call BuildPackage,rpi-hellojpeg))
$(eval $(call BuildPackage,rpi-helloteapot))
$(eval $(call BuildPackage,rpi-hellotiger))
$(eval $(call BuildPackage,rpi-hellotriangle))
$(eval $(call BuildPackage,rpi-hellotriangle2))
$(eval $(call BuildPackage,rpi-hellovideo))
$(eval $(call BuildPackage,rpi-hellovideocube))
$(eval $(call BuildPackage,rpi-helloworld))
