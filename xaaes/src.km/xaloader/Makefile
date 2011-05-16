#
# Makefile for XaAES loader
#
TARGET = xaloader.prg

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = xaloader

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
LD = $(CC) $(CFLAGS) -nostdlib -Wl,--entry -Wl,__start
LIBS = -lgcc
CPU = 000
STRIPFLAGS =

# default definitions
OBJS = $(COBJS:.c=.o) $(SOBJS:.s=.o) $(SSOBJS:.S=.o)
GENFILES = $(TARGET)


$(TARGET): $(OBJS)
	$(LD) -o $@ -Wall $^ $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
