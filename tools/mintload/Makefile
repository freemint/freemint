#
# Makefile for MiNT loader
#
TARGET = mintload.prg

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = mintload

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
LD = $(CC) $(CFLAGS) -nostdlib -Wl,--entry -Wl,__start
LIBS = -lgcc
CPU = 000
STRIPFLAGS =

# default definitions
OBJS = $(COBJS:.c=.o) $(SOBJS:.S=.o)
GENFILES = $(TARGET)


$(TARGET): $(OBJS)
	$(LD) -o $@ -Wall $^ $(LIBS)


include $(top_srcdir)/DEPENDENCIES
