#
# Makefile for mkfatfs and gemkfatfs
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = mkfatfs

default: help

include $(srcdir)/MKFATFSDEFS

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
	@echo '# - $(mkfatfstargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(mkfatfstargets); do \
		(set -x; \
		($(STRIP) .compile_$$i/*.app .compile_$$i/mkfatfs) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(mkfatfstargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(mkfatfstargets):
	$(MAKE) buildmkfatfs mkfatfs=$@

#
# multi target stuff
#

ifneq ($(mkfatfs),)

compile_dir = .compile_$(mkfatfs)
mkfatfstarget = _stmp_$(mkfatfs)
realtarget = $(mkfatfstarget)

$(mkfatfstarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildmkfatfs: $(realtarget)
