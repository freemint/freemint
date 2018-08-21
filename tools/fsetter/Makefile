#
# Makefile for fsetter
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = fsetter

default: all-here

include $(srcdir)/FSETTERDEFS

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
	for i in $(fsettertargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(fsettertargets):
	$(MAKE) buildfsetter fsetter=$@

#
# multi target stuff
#

ifneq ($(fsetter),)

compile_dir = .compile_$(fsetter)
fsettertarget = _stmp_$(fsetter)
realtarget = $(fsettertarget)

$(fsettertarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildfsetter: $(realtarget)
