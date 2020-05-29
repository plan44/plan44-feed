AC_INIT([p44extdevmotorexp], [0.1], [luz@plan44.ch], [p44extdevmotorexp], [http://www.plan44.ch/])
AC_PREREQ([2.59])

AC_CONFIG_AUX_DIR(configure_aux)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.10 no-define foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX

AC_CONFIG_FILES([Makefile])

##### OPTIONS

AC_ARG_ENABLE(
  [debug],
  [AC_HELP_STRING([--enable-debug], [Debug build with extra debugging code])]
)
AM_CONDITIONAL([DEBUG], [test "x$enable_debug" = xyes])

AC_ARG_ENABLE(
  [p44buildow],
  [AC_HELP_STRING([--enable-p44buildow], [plan44 build for openwrt targets])]
)
AM_CONDITIONAL([P44_BUILD_OW], [test "x$enable_p44buildow" = xyes])



AC_CHECK_LIB(m, atan2, [], [AC_MSG_ERROR([Could not find math lib (m) with atan2])])
AC_CHECK_LIB(rt, clock_gettime, [], [AC_MSG_ERROR([Could not find rt lib with clock_gettime])])
AC_CHECK_LIB(dl, dlopen, [], [AC_MSG_ERROR([Could not find libdl with dlopen])])

AC_CHECK_LIB(json-c, json_tokener_get_error, [], [AC_MSG_ERROR([Could not find JSON-C / libjson0 with json_tokener_get_error supported (>=0.10)])])

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([JSONC], [json-c >= 0.10 ], [], [
  AC_MSG_ERROR([$JSON_PKG_ERRORS])
])


# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h sys/resource.h], [], [AC_MSG_ERROR([required system header not found])])


ASSUMED_BOOST_VER=146

BOOST_REQUIRE([1.46], [
  unset boost_cv_version
  unset boost_cv_inc_path
  BOOST_REQUIRE([1.35], [
    AC_MSG_ERROR([could not find boost on your system])
  ])
  ASSUMED_BOOST_VER=135
])

if test $ASSUMED_BOOST_VER -eq 135; then
  AC_DEFINE([BOOST_VERSION_135], [1], [boost 1.35 or higher])
else
  AC_DEFINE([BOOST_VERSION_146], [1], [boost 1.46 or higher])
fi

BOOST_BIND
BOOST_FOREACH
BOOST_FUNCTION
BOOST_SMART_PTR

AX_PTHREAD([], [ AC_MSG_ERROR([required pthread library not found]) ])

AC_OUTPUT
