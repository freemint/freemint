#
# Makefile for net-tools
#

SHELL = /bin/sh
SUBDIRS = slinkctl tests

srcdir = .
top_srcdir = ..
subdir = net-tools

default: all-here

include $(srcdir)/NETTOOLSDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)


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
