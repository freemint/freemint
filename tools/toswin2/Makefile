#
# Makefile for TosWin 2
#
TARGET = toswin2.app

SHELL = /bin/sh
SUBDIRS = tw-call

srcdir = .
top_srcdir = ..
subdir = toswin2

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES += -I/usr/GEM/include

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -L/usr/GEM/lib -lcflib -lgem
LIBS += -liio
GENFILES = $(TARGET)


$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
