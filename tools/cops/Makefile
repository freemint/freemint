#
# Makefile for cops
#
TARGET = cops.app

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = cops

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
DEFINITIONS += -D__GEMLIB_OLDNAMES

NOCFLAGS-cops_rsc.c = -Wall

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -lgem -liio
GENFILES = $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
