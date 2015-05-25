#
# Makefile for 
#
TARGET = inet4.xdd

SHELL = /bin/sh
SUBDIRS = inet4 xif

srcdir = .
top_srcdir = ..
subdir = sockets

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I. -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XDD__

LD = $(CC) -mshort -nostdlib -Wl,--entry -Wl,_init
LIBS = -Linet4 -linet4 $(LIBKERN) -lgcc
ifndef CPU
CPU = 000
endif

# default definitions
SGENFILES = $(TARGET)
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
