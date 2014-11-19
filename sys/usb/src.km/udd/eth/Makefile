#
# Makefile for udd module
#
TARGET = eth.udd

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = eth

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/sockets
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=eth $(XDD_DEFINITIONS)
XDD_DEFINITIONS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 000

# default definitions
GENFILES = $(TARGET)
OBJS = $(COBJS:.c=.o) $(SSOBJS:.S=.o) 

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

include $(top_srcdir)/DEPENDENCIES
