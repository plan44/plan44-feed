AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

bin_PROGRAMS = p44extdevelsner

if DEBUG
p44extdevelsner_DEBUG = -D DEBUG=1
else
p44extdevelsner_DEBUG =
endif

if P44_BUILD_OW
p44extdevelsner_PLATFORM = -D P44_BUILD_OW=1
else
p44extdevelsner_PLATFORM =
endif

p44extdevelsner_EXTRACFLAGS = -D BOOST_BIND_GLOBAL_PLACEHOLDERS=1

p44extdevelsner_LDADD = $(PTHREAD_LIBS) $(JSONC_LIBS) -ldl


p44extdevelsner_CXXFLAGS = \
  -I ${srcdir}/src/p44utils \
  -I ${srcdir}/src \
  ${BOOST_CPPFLAGS} \
  ${JSONC_CFLAGS} \
  ${PTHREAD_CFLAGS} \
  ${p44extdevelsner_EXTRACFLAGS} \
  ${p44extdevelsner_PLATFORM} \
  ${p44extdevelsner_DEBUG}

p44extdevelsner_SOURCES = \
  src/p44utils/p44obj.cpp \
  src/p44utils/p44obj.hpp \
  src/p44utils/application.cpp \
  src/p44utils/application.hpp \
  src/p44utils/consolekey.cpp \
  src/p44utils/consolekey.hpp \
  src/p44utils/error.cpp \
  src/p44utils/error.hpp \
  src/p44utils/jsoncomm.cpp \
  src/p44utils/jsoncomm.hpp \
  src/p44utils/jsonobject.cpp \
  src/p44utils/jsonobject.hpp \
  src/p44utils/logger.cpp \
  src/p44utils/logger.hpp \
  src/p44utils/mainloop.cpp \
  src/p44utils/mainloop.hpp \
  src/p44utils/fdcomm.cpp \
  src/p44utils/fdcomm.hpp \
  src/p44utils/socketcomm.cpp \
  src/p44utils/socketcomm.hpp \
  src/p44utils/serialcomm.cpp \
  src/p44utils/serialcomm.hpp \
  src/p44utils/utils.cpp \
  src/p44utils/utils.hpp \
  src/p44utils/p44utils_common.hpp \
  src/p44utils_config.hpp \
  src/main.cpp
