#
# Makefile for a xdd skeleton
#
TARGET = dummy.xdd

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = skeleton

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = 

LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 000
# CPU = 020-60

# default definitions
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
