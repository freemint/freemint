#
# Makefile for ucd module
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = ethernat

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: entry

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=ethernat $(XDD_DEFINITIONS)
XDD_DEFINITIONS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 060

# default definitions
SGENFILES = ethernat.ucd
OBJS = $(SSOBJS:.S=.o)


entry:
	$(MAKE) ethernat.ucd
	
ethernat.ucd: isp116x-hcd.o $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $< $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
