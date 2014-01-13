#
# Makefile for ucd module
#
TARGET = unicorn.ucd

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = unicorn

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=unicorn $(XDD_DEFINITIONS)
XDD_DEFINITIONS = 
CPU = 000

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc

# default definitions
SGENFILES = $(TARGET)
OBJS = $(SSOBJS:.S=.o) $(COBJS:.c=.o)
ACSI_OBJS = $(addsuffix .o)

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

include $(top_srcdir)/DEPENDENCIES
