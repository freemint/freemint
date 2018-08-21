#
# Makefile for hyp_view (hyp)
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = hyp

default: all-here

include $(srcdir)/HYPDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) libhyp*.a


all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(hyptargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(hyptargets):
	$(MAKE) buildhyp hyp=$@

#
# multi target stuff
#

ifneq ($(hyp),)

compile_dir = .compile_$(hyp)
hyptarget = _stmp_$(hyp)
realtarget = $(hyptarget)

$(hyptarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildhyp: $(realtarget)
