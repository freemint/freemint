#
# Makefile for I/O abstraction library
#
TARGET = libIO.a

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = IO

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites

# default definitions
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS)
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@


include $(top_srcdir)/DEPENDENCIES
