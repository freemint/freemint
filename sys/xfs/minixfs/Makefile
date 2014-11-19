#
# Makefile for 
#
TARGET = minix.xfs

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = minixfs

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XFS__

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 000
STRIPFLAGS = $(shell for i in `cat syms.mfs` ; do echo -n "-K $$i " ; done)

# default definitions
GENFILES = $(TARGET)
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
