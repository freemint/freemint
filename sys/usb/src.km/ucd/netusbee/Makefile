#
# Makefile for netusbee
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = netusbee
toscommon_srcdir = ../../tos-common

default:

include ./NETUSBEEDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY


all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.ucd *.prg
SGENFILES = crtinit.c delayasm.S entry.S setstack.S

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(netusbeetargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(netusbeetargets):
	$(MAKE) buildnetusbee netusbee=$@

#
# multi target stuff
#

ifneq ($(netusbee),)

compile_dir = .compile_$(netusbee)
netusbeetarget = _stmp_$(netusbee)
realtarget = $(netusbeetarget)

$(netusbeetarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

# Link common TOS files
ifeq ($(netusbee),prg)
	ln -s $(toscommon_srcdir)/crtinit.c $(srcdir)/crtinit.c; \
	ln -s $(toscommon_srcdir)/delayasm.S $(srcdir)/delayasm.S; \
	ln -s $(toscommon_srcdir)/entry.S $(srcdir)/entry.S; \
	ln -s $(toscommon_srcdir)/setstack.S $(srcdir)/setstack.S
endif

else

realtarget =

endif

buildnetusbee: $(realtarget)
