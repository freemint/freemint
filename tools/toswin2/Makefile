#
# Makefile for TosWin 2
#
TARGET = toswin2.app

SHELL = /bin/sh
SUBDIRS = tw-call

srcdir = .
top_srcdir = ..
subdir = toswin2

installdir = /opt/GEM/toswin2

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES += -I/usr/GEM/include
CFLAGS += -D_GNU_SOURCE
#CFLAGS += -DDEBUG
CFLAGS += -g
#CFLAGS += -DONLY_XAAES

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -L/usr/GEM/lib -lcflib -lgem
GENFILES = $(TARGET)

MEMDEBUG = no
ifeq ($(MEMDEBUG), yes)
CFLAGS += -DMEMDEBUG=\"/tmp/memdebug.txt\" -O0
LDFLAGS += -Wl,--wrap -Wl,malloc -Wl,--wrap -Wl,realloc \
-Wl,--wrap -Wl,calloc -Wl,--wrap -Wl,free
endif

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

include $(top_srcdir)/DEPENDENCIES

install: all
	$(top_srcdir)/mkinstalldirs $(installdir)
	cp $(TARGET) $(srcdir)/toswin2.rsc $(srcdir)/toswin2.hrd $(installdir)
	chmod 755 $(installdir)/$(TARGET)
	$(STRIP) $(installdir)/$(TARGET)
	
