#
# Makefile for 
#
TARGET = libksocket16.a

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../..
subdir = ksocket

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = 
CPU = 000

# default definitions
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@


include $(top_srcdir)/DEPENDENCIES
