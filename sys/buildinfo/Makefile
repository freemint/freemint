#
# kernel buildinfo things
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = buildinfo

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: mkbuild

# default overwrites
CC = $(NATIVECC)
CFLAGS = $(NATIVECFLAGS)

# default definitions
OBJS = mkbuild.o
GENFILES = mkbuild buildserial
SGENFILES = build.h cdef.h

mkbuild: mkbuild.o
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# default dependencies
# must be included last
include $(top_srcdir)/DEPENDENCIES
