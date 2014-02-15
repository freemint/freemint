#
# Makefile for udd module
#
TARGET = storage.udd

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = storage

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=storage $(XDD_DEFINITIONS)
XDD_DEFINITIONS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
ifndef CPU
CPU = 000
endif

# default definitions
SGENFILES = $(TARGET)
OBJS = $(COBJS:.c=.o) $(SSOBJS:.S=.o) 

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

include $(top_srcdir)/DEPENDENCIES
