AC_INIT([p44devd], [0.1], [luz@plan44.ch], [p44devd], [http://www.plan44.ch/])
AC_PREREQ([2.59])

AC_CONFIG_AUX_DIR(configure_aux)

AM_INIT_AUTOMAKE([1.10 no-define foreign])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX

AC_CONFIG_FILES([Makefile])

PKG_PROG_PKG_CONFIG

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h string.h sys/ioctl.h unistd.h], [], [AC_MSG_ERROR([required system header not found])])

AC_CHECK_LIB(ubus, ubus_connect_ctx, [], [AC_MSG_ERROR([Could not find libubus with ubus_connect_ctx])])
AC_CHECK_LIB(ubox, uloop_init, [], [AC_MSG_ERROR([Could not find libubox with uloop_init])])
AC_CHECK_LIB(blobmsg_json, blobmsg_add_object, [], [AC_MSG_ERROR([Could not find libblobmsg_json with blobmsg_add_object])])

AC_OUTPUT
