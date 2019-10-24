#
# Makefile for xaaes
#

SHELL = /bin/sh
SUBDIRS = src.km

srcdir = .
top_srcdir = ..
subdir = xaaes

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here:

# default overwrites

# default definitions

help 000 020 030 040 060 deb mil col sockets xdd xfs all-kernels:
	for i in $(SUBDIRS); do $(MAKE) -C $$i $@; done

ara:: 040
	@:

hat:: 000
	@:
