#
# Makefile for 
#
TARGET = xaaes.prg

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = .
subdir = src

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
DEFINITIONS = $(XAAESDEFAULTDEFS)
LD = $(CC) -nostdlib -Wl,--entry -Wl,__start
LIBS = -lgem16 $(LIBKERN) -lgcc
CPU = 000
STRIPFLAGS = 

# default definitions
SGENFILES = $(TARGET)
OBJS = $(COBJS:.c=.o) $(SOBJS:.s=.o) $(SSOBJS:.S=.o)


$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
