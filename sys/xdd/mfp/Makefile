#
# Makefile for mfp
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = mfp

default: all

CPU = 000

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: entry

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XDD__ $(XDD_DEFINITIONS)
XDD_DEFINITIONS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc

# default definitions
GENFILES = mfp.xdd mfp_mil.xdd
SGENFILES = mfp_mil.c
OBJS = $(COBJS:.c=.o)

entry:
	$(MAKE) XDD_DEFINITIONS="" CPU="000" mfp.xdd
	$(MAKE) XDD_DEFINITIONS="-DMILAN" CPU="020-60" mfp_mil.xdd

mfp.xdd: mfp.o $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $< $(LIBS)

mfp_mil.xdd: mfp_mil.o $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $< $(LIBS)

mfp_mil.c:
	ln -s mfp.c mfp_mil.c


include $(top_srcdir)/DEPENDENCIES
