#
# Makefile for unicorn
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = unicorn
toscommon_srcdir = ../../tos-common

default:

include ./UNICORNDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY


all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.ucd *.prg
SGENFILES = crtinit.c entry.S setstack.S

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(unicorntargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(unicorntargets): 
	$(MAKE) buildunicorn unicorn=$@

#
# multi target stuff
#

ifneq ($(unicorn),)

compile_dir = .compile_$(unicorn)
unicorntarget = _stmp_$(unicorn)
realtarget = $(unicorntarget)

$(unicorntarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

# Link common TOS files
ifeq ($(unicorn),prg)
	ln -s $(toscommon_srcdir)/crtinit.c $(srcdir)/crtinit.c; \
	ln -s $(toscommon_srcdir)/entry.S $(srcdir)/entry.S; \
	ln -s $(toscommon_srcdir)/setstack.S $(srcdir)/setstack.S
endif

else

realtarget =

endif

buildunicorn: $(realtarget)
