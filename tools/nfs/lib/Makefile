#
# Makefile for 
#
TARGET = librpc16.a

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = lib

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
GENERAL = -mshort

# default definitions
OBJS = $(COBJS:.c=.o)
GENFILES = $(TARGET)

$(TARGET): $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@


include $(top_srcdir)/DEPENDENCIES
