#
# Makefile for mouse (boot protocol)
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../../..
subdir = mouse-boot
toscommon_srcdir = ../../../tos-common

default:

include ./MOUSEDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

SGENFILES = crtinit.c entry.S setstack.S

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.udd *.prg

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(mousetargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(mousetargets): $(SGENFILES)
	$(MAKE) buildmouse mouse=$@

# Link common TOS files
crtinit.c: $(toscommon_srcdir)/crtinit.c
	ln -sf $< $(srcdir)/$@
entry.S: $(toscommon_srcdir)/entry.S
	ln -sf $< $(srcdir)/$@
setstack.S: $(toscommon_srcdir)/setstack.S
	ln -sf $< $(srcdir)/$@

#
# multi target stuff
#

ifneq ($(mouse),)

compile_dir = .compile_$(mouse)
mousetarget = _stmp_$(mouse)
realtarget = $(mousetarget)

$(mousetarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildmouse: $(realtarget)
