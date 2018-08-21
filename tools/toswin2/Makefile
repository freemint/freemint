#
# Makefile for TosWin 2
#

SHELL = /bin/sh
SUBDIRS = tw-call

srcdir = .
top_srcdir = ..
subdir = toswin2

default: all-here

include $(srcdir)/TOSWIN2DEFS

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
	for i in $(toswin2targets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(toswin2targets):
	$(MAKE) buildtoswin2 toswin2=$@

#
# multi target stuff
#

ifneq ($(toswin2),)

compile_dir = .compile_$(toswin2)
toswin2target = _stmp_$(toswin2)
realtarget = $(toswin2target)

$(toswin2target): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildtoswin2: $(realtarget)
