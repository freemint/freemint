#
# Makefile for ext2
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = 

default: help

include ./EXT2DEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY


all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.km
SGENFILES = xversion.h


help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(ext2targets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '#'

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(ext2targets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(ext2targets): xversion.h
	$(MAKE) buildext2 ext2=$@

xversion.h: $(SRCFILES)
	$(SHELL) xversion.sh $@ $?

#
# multi target stuff
#

ifneq ($(ext2),)

compile_dir = .compile_$(ext2)
ext2target = _stmp_$(ext2)
realtarget = $(ext2target)

$(ext2target): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildext2: $(realtarget)
