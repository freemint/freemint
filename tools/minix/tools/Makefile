#
# Makefile for minixfs tools
#
TARGET = flist mfsconf

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = tools

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites

# default definitions
OBJS = $(COBJS:.c=.o)
GENFILES = $(TARGET)

FLISTOBJS = flist.o
MFSCONFOBJS = mfsconf.o

flist: $(FLISTOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(FLISTOBJS) $(LIBS)
	$(STRIP) $@

mfsconf: $(MFSCONFOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(MFSCONFOBJS) $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
