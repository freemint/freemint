#
# Makefile for 
#
TARGET = mount_nfs

SHELL = /bin/sh
SUBDIRS = lib

srcdir = .
top_srcdir = ..
subdir = nfs

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
OPTS = -mshort
INCLUDES = -Ilib

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -Llib -lrpc16 -lsocket16
GENFILES = $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
