#
# Makefile for minixfs
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = minixfs

default: help

include ./MINIXDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.xfs

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(minixtargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '#'

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(minixtargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(minixtargets):
	$(MAKE) buildminix minix=$@

#
# multi target stuff
#

ifneq ($(minix),)

compile_dir = .compile_$(minix)
minixtarget = _stmp_$(minix)
realtarget = $(minixtarget)

$(minixtarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildminix: $(realtarget)
