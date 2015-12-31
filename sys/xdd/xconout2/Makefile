#
# Makefile for xconout2
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = xconout2

default: help

include ./XCONOUT2DEFS

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
	@echo '# - $(xconout2targets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '#'

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(xconout2targets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(xconout2targets): 
	$(MAKE) buildxconout2 xconout2=$@

#
# multi target stuff
#

ifneq ($(xconout2),)

compile_dir = .compile_$(xconout2)
xconout2target = _stmp_$(xconout2)
realtarget = $(xconout2target)

$(xconout2target): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildxconout2: $(realtarget)
