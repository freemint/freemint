#
# Makefile for 
#
TARGET = mfp.xdd

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = uart

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: entry

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 000

# default definitions
OBJS = $(COBJS:.c=.o)

entry:
	$(MAKE) DEFINITIONS="" CPU="000" mfp.xdd
	$(MAKE) DEFINITIONS="-DMILAN" CPU="020-60" mfp_mil.xdd
	$(MAKE) DEFINITIONS="-DTT_MFP" CPU="030" mfp_tt.xdd

mfp.xdd: mfp.c mfp.h $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $< $(LIBS)
	$(STRIP) $@

mfp_tt.xdd: mfp.c mfp.h $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $< $(LIBS)
	$(STRIP) $@

mfp_mil.xdd: mfp.c mfp.h $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $< $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
