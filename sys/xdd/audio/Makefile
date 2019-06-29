#
# Makefile for 
#
TARGET = audiodev.xdd

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = audio

default: all

CPU = 000

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET) actrl

strip:
	$(STRIP) $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XDD__

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc

# default definitions
GENFILES = $(TARGET) actrl
OBJS = $(COBJS:.c=.o) $(SOBJS:.S=.o)

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

actrl:
	$(CC) $(MODEL) $(OPTS) $(WARN) actrl.c -o $@

include $(top_srcdir)/DEPENDENCIES
