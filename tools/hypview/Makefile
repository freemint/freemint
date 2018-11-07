#
# Makefile for hyp_view
#

SHELL = /bin/sh
SUBDIRS = hyp plain dragdrop bubble

srcdir = .
top_srcdir = ..
subdir = hypview

default: all-here

include $(srcdir)/HYPVIEWDEFS

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
	for i in $(hypviewtargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(hypviewtargets):
	$(MAKE) buildhypview hypview=$@

#
# multi target stuff
#

ifneq ($(hypview),)

compile_dir = .compile_$(hypview)
hypviewtarget = _stmp_$(hypview)
realtarget = $(hypviewtarget)

$(hypviewtarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildhypview: $(realtarget)
