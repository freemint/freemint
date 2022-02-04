#
# Makefile for moose
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = whlmoose

default: help

include ./MOOSEDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) moose.adi moose_w.adi
SGENFILES = whlmoose_w.c

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(moosetargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(moosetargets); do \
		(set -x; \
		($(STRIP) .compile_$$i/*.adi) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(moosetargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(moosetargets): 
	$(MAKE) buildmoose moose=$@

#
# multi target stuff
#

ifneq ($(moose),)

compile_dir = .compile_$(moose)
moosetarget = _stmp_$(moose)
realtarget = $(moosetarget)

$(moosetarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildmoose: $(realtarget)
