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
DEFINITIONS = -D__KERNEL_XDD__ $(XDD_DEFINITIONS)
XDD_DEFINITIONS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 000

# default definitions
SGENFILES = $(TARGET) mfp_mil.xdd
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
