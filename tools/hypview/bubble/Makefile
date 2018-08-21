#
# Makefile for hyp_view (bubble)
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = bubble

default: all-here

include $(srcdir)/BUBBLEDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) libbgh*.pdb libbgh*.a


all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(bubbletargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(bubbletargets):
	$(MAKE) buildbubble bubble=$@

#
# multi target stuff
#

ifneq ($(bubble),)

compile_dir = .compile_$(bubble)
bubbletarget = _stmp_$(bubble)
realtarget = $(bubbletarget)

$(bubbletarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildbubble: $(realtarget)
