#
# Makefile for ATARI and DOS fdisk
#
TARGET = fdisk sfdisk

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = fdisk

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)/IO

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += $(LIBIO)
GENFILES = $(TARGET)

FDISKOBJS = fdisk.o
SFDISKOBJS = sfdisk.o

fdisk: $(FDISKOBJS) $(LIBIOSTMP)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

sfdisk: $(SFDISKOBJS) $(LIBIOSTMP)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)


include $(top_srcdir)/DEPENDENCIES
