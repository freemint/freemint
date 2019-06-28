#
# Makefile for minixfs initialization
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = minit

default: help

include $(srcdir)/MINITDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(minittargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(minittargets); do \
		(set -x; \
		($(STRIP) .compile_$$i/minit) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(minittargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(minittargets):
	$(MAKE) buildminit minit=$@

#
# multi target stuff
#

ifneq ($(minit),)

compile_dir = .compile_$(minit)
minittarget = _stmp_$(minit)
realtarget = $(minittarget)

$(minittarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildminit: $(realtarget)
