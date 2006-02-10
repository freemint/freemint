#
# Makefile for hyp_view (plain)
#
TARGET = libplain.a

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = plain

installdir =

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
CFLAGS += -g

# default definitions
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS)
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

include $(top_srcdir)/DEPENDENCIES
	