#
# Makefile for 
#
TARGET = xaaes.km

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = .
subdir = src.km

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET) load.prg

# default overwrites
INCLUDES += -I../../sys
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=XaAES $(XAAESDEFAULTDEFS)
OPTS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = -lgem16 $(LIBKERN) -lgcc
CPU = 000
STRIPFLAGS = 

# default definitions
SGENFILES = $(TARGET) test.km load.prg
OBJS = $(COBJS:.c=.o) $(SOBJS:.s=.o) $(SSOBJS:.S=.o)


$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

load.prg: load.c
	$(CC) -o $@ -Wall $<


include $(top_srcdir)/DEPENDENCIES
