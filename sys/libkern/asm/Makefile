#
# Makefile for libkern asm-m68k part
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = asm

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: 

# default overwrites

# default definitions
GENFILES = .000 .030 .020-60

#
# main target
#
asm-files: .$(CPU)
	cd .$(CPU); $(MAKE)

.$(CPU):
	$(MKDIR) $@
	$(CP) Makefile.asm-files $@/Makefile
