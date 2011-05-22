#
# Makefile for moose
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = netusbee

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: entry

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=netusbee $(XDD_DEFINITIONS) -DUSB_SUPPORT
XDD_DEFINITIONS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 030

# default definitions
SGENFILES = netusbee.ucd
OBJS = $(SSOBJS:.s=.o)


entry:
	$(MAKE) netusbee.ucd
	
netusbee.ucd: isp116x-hcd.o $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $< $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
