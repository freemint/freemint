#
# Makefile for 
#
TARGET = sockdev.xdd

SHELL = /bin/sh
SUBDIRS = inet4 unix xif

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
DEFINITIONS = 

LD = $(CC) -mshort -nostdlib -Wl,--entry -Wl,_init
LIBS = -Linet4 -Lunix -linet4 -lunix $(LIBKERN) -lgcc
CPU = 000

# default definitions
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
