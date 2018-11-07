#
# Makefile for hyp_view (plain)
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = plain

default: all-here

include $(srcdir)/PLAINDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) libplain*.a


all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(plaintargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(plaintargets):
	$(MAKE) buildplain plain=$@

#
# multi target stuff
#

ifneq ($(plain),)

compile_dir = .compile_$(plain)
plaintarget = _stmp_$(plain)
realtarget = $(plaintarget)

$(plaintarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildplain: $(realtarget)
