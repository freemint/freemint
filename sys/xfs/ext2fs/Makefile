#
# Makefile for
#
TARGET = ext2.xfs

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = src

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
CPU = 020-60
#CPU = 000
STRIPFLAGS =

# default definitions
SGENFILES = $(TARGET)
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
