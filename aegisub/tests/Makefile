# $Id$
include ../Makefile.inc

PROGRAM = run

LIBS += -L../libaegisub -laegisub  -lgtest -L../universalchardet -luniversalchardet
LDFLAGS += -Wl,-rpath $(CURDIR)/../libaegisub $(LDFLAGS_ICONV)

CPPFLAGS += -I../src/include -I../libaegisub/include $(CFLAGS_ICONV)
CXXFLAGS += -Wno-unused-value

ifeq (yes, $(BUILD_DARWIN))
LDFLAGS += -framework ApplicationServices
endif

SRC = \
		main.cpp \
		util.cpp \
		util_unix.cpp \
		libaegisub_color.cpp
		
HEADER = \
	*.h

include ../Makefile.target
