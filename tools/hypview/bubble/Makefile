#
# Makefile for hyp_view (plain)
#
TARGET = libbgh.a

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = dragdrop

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
GENFILES = $(TARGET) libbgh.pdb libbgh.lib

$(TARGET): $(OBJS)
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

include $(top_srcdir)/DEPENDENCIES
	