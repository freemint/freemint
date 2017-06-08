#
# Makefile for MiNT loader
#
TARGET = usb.acc

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = usb

default: all

CPU = 000

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
CFLAGS += -fno-builtin -I../../sys
LD = $(CC) $(CFLAGS) -nostdlib -Wl,--entry -Wl,__start
LIBS = -lgcc -lgem
STRIPFLAGS =

# default definitions
OBJS = $(COBJS:.c=.o) $(SOBJS:.s=.o) $(SSOBJS:.S=.o)
GENFILES = $(TARGET)


$(TARGET): $(OBJS)
	$(LD) -o $@ -Wall $^ $(LIBS)


include $(top_srcdir)/DEPENDENCIES
