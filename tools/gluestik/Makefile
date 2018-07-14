#
# Makefile for gluestik, a stik emulator for mintnet
#
TARGET = gluestik.prg

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = gluestik

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites

# default definitions
OBJS = $(COBJS:.c=.o)
GENFILES = $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -Wl,--mprg-flags,0x17


include $(top_srcdir)/DEPENDENCIES
