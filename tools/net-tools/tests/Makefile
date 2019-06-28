#
# Makefile for test tools
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = tests

default: help

include $(srcdir)/TESTSDEFS

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
	@echo '# - $(teststargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(teststargets); do \
		(set -x; \
		($(STRIP) .compile_$$i/client .compile_$$i/dgram .compile_$$i/dgramd \
		.compile_$$i/hostlookup .compile_$$i/oobcl .compile_$$i/oobsv \
		.compile_$$i/pipes .compile_$$i/protolookup .compile_$$i/server \
		.compile_$$i/servlookup .compile_$$i/sockname .compile_$$i/sockpair \
		.compile_$$i/speed .compile_$$i/speed2 .compile_$$i/speedd \
		.compile_$$i/tcpcl .compile_$$i/tcpsv .compile_$$i/udpclnt \
		.compile_$$i/udpserv ) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(teststargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(teststargets):
	$(MAKE) buildtests tests=$@

#
# multi target stuff
#

ifneq ($(tests),)

compile_dir = .compile_$(tests)
teststarget = _stmp_$(tests)
realtarget = $(teststarget)

$(teststarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildtests: $(realtarget)
