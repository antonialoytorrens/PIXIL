# apps/find/Makefile

TARGET_CXX=gfind

SRC=${shell ls *.cxx} 
OBJS=${SRC:.cxx=.o}

# This is for FLEK 
CFLAGS ?=
CFLAGS +=-DPIXIL

LIBS=-lpixil-pim

ifeq ($(CONFIG_NANOX),y)
LIBS += -lwm
endif

ifeq ($(CONFIG_PAR),y)
LIBS+=-lpar
endif

ifeq ($(CONFIG_SYNC),y)
LIBS+=-lpixil-sync
endif

LIBS+=-lfltk -lflek

ifeq ($(CONFIG_NANOX),y)
LIBS+=-lnano-X 
endif

ifeq ($(CONFIG_X11),y)
LIBS+=-lX11
endif

ifeq ($(CONFIG_COLOSSEUM),y)
LIBS+=-lipc
endif

include $(BASE_DIR)/Rules.make

