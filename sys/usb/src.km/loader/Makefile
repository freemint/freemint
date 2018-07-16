#
# Makefile for usb loader
#

TARGETS = loader.prg #unloader.prg

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../..
subdir = loader
toscommon_srcdir = ../tos-common

default: all

ifndef CPU
CPU = 000
endif

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

SGENFILES = crtinit.c entry.S setstack.S

all-here: $(TARGETS)

# default overwrites
LD = $(CC) -nostdlib -Wl,--entry -Wl,__start
LIBS = -lgcc
STRIPFLAGS =
INCLUDES = 
DEFINITIONS = -DSTACKSIZE=2048

# default definitions
OBJS = $(COBJS:.c=.o) $(SOBJS:.s=.o) $(SSOBJS:.S=.o)
GENFILES = $(TARGETS)

#COMMON		= crtinit.o $(SSOBJS:.S=.o)
#LOADER		= loader.o $(COMMON)
#UNLOADER	= unloader.o $(COMMON)

$(TARGETS): $(OBJS)
	$(LD) -o $@ -Wall $^ $(LIBS)
	$(STRIP) $@

# Link common TOS files
crtinit.c: $(toscommon_srcdir)/crtinit.c
	ln -sf $< $(srcdir)/$@
entry.S: $(toscommon_srcdir)/entry.S
	ln -sf $< $(srcdir)/$@
setstack.S: $(toscommon_srcdir)/setstack.S
	ln -sf $< $(srcdir)/$@

include $(top_srcdir)/DEPENDENCIES
