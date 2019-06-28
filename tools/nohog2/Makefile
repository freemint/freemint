#
# Makefile for NoHog2
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = nohog2

default: all-here

include $(srcdir)/NOHOG2DEFS

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
	@echo '# - $(nohog2targets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(nohog2targets); do \
		(set -x; \
		($(STRIP) .compile_$$i/*.acc) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(nohog2targets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(nohog2targets):
	$(MAKE) buildnohog2 nohog2=$@

#
# multi target stuff
#

ifneq ($(nohog2),)

compile_dir = .compile_$(nohog2)
nohog2target = _stmp_$(nohog2)
realtarget = $(nohog2target)

$(nohog2target): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildnohog2: $(realtarget)
