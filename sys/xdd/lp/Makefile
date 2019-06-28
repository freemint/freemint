#
# Makefile for lp
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = lp

default: help

include ./LPDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.xdd

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(lptargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(lptargets); do \
		(set -x; \
		($(STRIP) .compile_$$i/*.xdd) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(lptargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(lptargets): 
	$(MAKE) buildlp lp=$@

#
# multi target stuff
#

ifneq ($(lp),)

compile_dir = .compile_$(lp)
lptarget = _stmp_$(lp)
realtarget = $(lptarget)

$(lptarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildlp: $(realtarget)
