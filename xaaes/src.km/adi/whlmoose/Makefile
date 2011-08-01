#
# Makefile for moose
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = whlmoose

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: entry

# default overwrites
INCLUDES = -I$(top_srcdir)/../../sys
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=whlmoose $(XDD_DEFINITIONS)
XDD_DEFINITIONS =

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 000

# default definitions
GENFILES = moose.adi moose_w.adi
SGENFILES = whlmoose_w.c
OBJS = $(SOBJS:.S=.o)


entry:
	$(MAKE) XDD_DEFINITIONS="" moose.adi
	$(MAKE) XDD_DEFINITIONS="-DHAVE_WHEEL=1" moose_w.adi

moose.adi: whlmoose.o $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $< $(OBJS) $(LIBS)

moose_w.adi: whlmoose_w.o $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $< $(OBJS) $(LIBS)

whlmoose_w.c: whlmoose.c
	ln -s $< $@


include $(top_srcdir)/DEPENDENCIES
