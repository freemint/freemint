#
# Makefile for hyp_view (dragdrop)
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = dragdrop

default: all-here

include $(srcdir)/DRAGDROPDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) libdgdp*.pdb libdgdp*.a


all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(dragdroptargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(dragdroptargets):
	$(MAKE) builddragdrop dragdrop=$@

#
# multi target stuff
#

ifneq ($(dragdrop),)

compile_dir = .compile_$(dragdrop)
dragdroptarget = _stmp_$(dragdrop)
realtarget = $(dragdroptarget)

$(dragdroptarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

builddragdrop: $(realtarget)
