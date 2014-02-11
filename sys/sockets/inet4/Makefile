#
# Makefile for 
#
TARGET = libinet4.a

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = 

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I.. -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XDD__
ifndef CPU
CPU = 000
endif

# default definitions
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS)
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@


include $(top_srcdir)/DEPENDENCIES
