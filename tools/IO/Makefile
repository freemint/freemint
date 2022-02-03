#
# Makefile for I/O abstraction library
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = IO

default: all-here

include $(srcdir)/IODEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) libIO*.a


all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(iotargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(iotargets):
	$(MAKE) buildio io=$@

#
# multi target stuff
#

ifneq ($(io),)

compile_dir = .compile_$(io)
iotarget = _stmp_$(io)
realtarget = $(iotarget)

$(iotarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildio: $(realtarget)
