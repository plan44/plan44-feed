AC_PREREQ([2.68])

AC_INIT([p44pagekite], [0.1], [info@plan44.ch], [p44pagekite], [http://www.plan44.ch/])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for header files.
AC_CHECK_HEADERS([pthread.h sched.h fcntl.h float.h inttypes.h limits.h stddef.h stdint.h stdlib.h string.h strings.h sys/time.h unistd.h])

# Need libpagekite
AC_CHECK_LIB(pagekite, pagekite_init, [], [AC_MSG_ERROR([Could not find libpagekite with pagekite_init])])
# and SSL
AC_CHECK_LIB(ssl, TLSv1_client_method, [], [AC_MSG_ERROR([Could not find libssl with TLSv1_client_method])])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
