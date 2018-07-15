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
GENFILES = .compile_*

#
# main target
#
asm-files: .compile_$(CPU)
	cd .compile_$(CPU); $(MAKE)

# prevent overriding the same recipe from RULES
ifneq ($(compile_dir),.compile_$(CPU))
.compile_$(CPU): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile
endif
