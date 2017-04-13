#
# Makefile for ucd module
#
TARGET = aranym.ucd

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = aranym

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=aranym $(XDD_DEFINITIONS)
XDD_DEFINITIONS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 020-60

# default definitions
SGENFILES = $(TARGET)
OBJS = $(SSOBJS:.S=.o) $(COBJS:.c=.o)

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

include $(top_srcdir)/DEPENDENCIES
