#
# Makefile for XaAES
#

SHELL = /bin/sh
SUBDIRS = adi xaloader gradient

srcdir = .
top_srcdir = .
subdir = src.km

default: help

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
	@echo '# - $(xaaestargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(xaaestargets); do \
		(set -x; \
		($(STRIP) .compile_$$i/*.km) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(xaaestargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(xaaestargets): xversion.h
	$(MAKE) buildxaaes xaaes=$@

xversion.h: $(SRCFILES)
	$(SHELL) xversion.sh $@ $?

#
# multi target stuff
#

ifneq ($(xaaes),)

compile_dir = .compile_$(xaaes)
xaaestarget = _stmp_$(xaaes)
realtarget = $(xaaestarget)

$(xaaestarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildxaaes: $(realtarget)
