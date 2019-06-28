#
# the keyboard table switcher
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = swkbdtbl

default: help

include $(srcdir)/SWKBDTBLDEFS

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
	@echo '# - $(swkbdtbltargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(swkbdtbltargets); do \
		(set -x; \
		($(STRIP) .compile_$$i/swkbdtbl) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(swkbdtbltargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(swkbdtbltargets):
	$(MAKE) buildswkbdtbl swkbdtbl=$@

#
# multi target stuff
#

ifneq ($(swkbdtbl),)

compile_dir = .compile_$(swkbdtbl)
swkbdtbltarget = _stmp_$(swkbdtbl)
realtarget = $(swkbdtbltarget)

$(swkbdtbltarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildswkbdtbl: $(realtarget)
