#
# Makefile for USB loader
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../..
subdir = loader
toscommon_srcdir = ../tos-common

default: all-here

include $(srcdir)/USBLOADDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)
SGENFILES = crtinit.c entry.S setstack.S

all-targets:
# Link common TOS files
	ln -sf $(toscommon_srcdir)/crtinit.c crtinit.c
	ln -sf $(toscommon_srcdir)/entry.S entry.S
	ln -sf $(toscommon_srcdir)/setstack.S setstack.S

	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(usbloadtargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(usbloadtargets):
	$(MAKE) buildusbload usbload=$@

#
# multi target stuff
#

ifneq ($(usbload),)

compile_dir = .compile_$(usbload)
usbloadtarget = _stmp_$(usbload)
realtarget = $(usbloadtarget)

$(usbloadtarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildusbload: $(realtarget)
