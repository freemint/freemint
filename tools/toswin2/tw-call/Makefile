#
# Makefile for tw-call, TosWin2 helper
#
TARGET = tw-call.app

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = tw-call

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES += -I/usr/GEM/include

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -L/usr/GEM/lib -lcflib -lgemx -lgem
LIBS += -liio
GENFILES = $(TARGET)


$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
