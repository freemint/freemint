#
# Makefile for mgw, a draconis TCP/IP emulator for mintnet
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = mgw

default: all-here

include $(srcdir)/MGWDEFS

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
	for i in $(mgwtargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(mgwtargets):
	$(MAKE) buildmgw mgw=$@

#
# multi target stuff
#

ifneq ($(mgw),)

compile_dir = .compile_$(mgw)
mgwtarget = _stmp_$(mgw)
realtarget = $(mgwtarget)

$(mgwtarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildmgw: $(realtarget)
