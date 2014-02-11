#
# Makefile for usb loader
#

TARGETS = loader.prg #unloader.prg

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../..
subdir = loader

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGETS)

# default overwrites
LD = $(CC) -nostdlib -Wl,--entry -Wl,__start
LIBS = -lgcc
ifndef CPU
CPU = 000
endif
STRIPFLAGS =
INCLUDES = 

# default definitions
OBJS = $(COBJS:.c=.o) $(SOBJS:.s=.o) $(SSOBJS:.S=.o)
GENFILES = $(TARGETS)

COMMON		= crtinit.o $(SSOBJS:.S=.o)
LOADER		= loader.o $(COMMON)
#UNLOADER	= unloader.o $(COMMON)

$(TARGETS): $(OBJS)
	$(LD) -o $@ -Wall $^ $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
