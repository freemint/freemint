#
# Makefile for mouse
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = mouse
toscommon_srcdir = ../../tos-common

default:

include ./MOUSEDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY


all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.udd *.prg
SGENFILES = crtinit.c entry.S setstack.S

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(mousetargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(mousetargets): 
	$(MAKE) buildmouse mouse=$@

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

# Link common TOS files
ifeq ($(mouse),prg)
	ln -s $(toscommon_srcdir)/crtinit.c $(srcdir)/crtinit.c; \
	ln -s $(toscommon_srcdir)/entry.S $(srcdir)/entry.S; \
	ln -s $(toscommon_srcdir)/setstack.S $(srcdir)/setstack.S
endif

else

realtarget =

endif

buildmouse: $(realtarget)
