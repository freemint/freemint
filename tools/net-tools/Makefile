#
# Makefile for net-tools
#

SHELL = /bin/sh
SUBDIRS = slinkctl tests

srcdir = .
top_srcdir = ..
subdir = net-tools

default: help

include $(srcdir)/NETTOOLSDEFS

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
	@echo '# - $(nettoolstargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(nettoolstargets); do \
		(set -x; \
		($(STRIP) .compile_$$i/arp .compile_$$i/diald .compile_$$i/ifconfig \
		.compile_$$i/iflink .compile_$$i/ifstats .compile_$$i/masqconf \
		.compile_$$i/netstat .compile_$$i/pppconf .compile_$$i/route \
		.compile_$$i/slattach) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(nettoolstargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(nettoolstargets):
	$(MAKE) buildnettools nettools=$@

#
# multi target stuff
#

ifneq ($(nettools),)

compile_dir = .compile_$(nettools)
nettoolstarget = _stmp_$(nettools)
realtarget = $(nettoolstarget)

$(nettoolstarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildnettools: $(realtarget)
