# $Id$
include ../Makefile.inc

PROGRAM = run

LIBS += -L../libaegisub -laegisub -L../universalchardet -luniversalchardet $(LIBS_BOOST) $(LIBS_ICU)
LDFLAGS += -Wl,-rpath $(CURDIR)/../libaegisub $(LDFLAGS_ICONV)

CPPFLAGS += -I../src/include -I../libaegisub/include $(CFLAGS_ICONV) -I${GTEST_ROOT} -I${GTEST_ROOT}/include -I./support $(CPPFLAGS_BOOST)
CXXFLAGS += -Wno-unused-value -Wno-sign-compare

ifeq (yes, $(BUILD_DARWIN))
LDFLAGS += -framework ApplicationServices -framework Foundation
endif

SRC = \
		support/main.cpp \
		support/util.cpp \
		support/util_unix.cpp \
		tests/dialogue_block.cpp \
		${GTEST_ROOT}/src/gtest-all.cc

HEADER = support/*.h tests/*.h

include ../Makefile.target
