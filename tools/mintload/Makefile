#
# Makefile for MiNT loader
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = mintload

default: all-here

include $(srcdir)/MINTLOADDEFS

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
	for i in $(mintloadtargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(mintloadtargets):
	$(MAKE) buildmintload mintload=$@

#
# multi target stuff
#

ifneq ($(mintload),)

compile_dir = .compile_$(mintload)
mintloadtarget = _stmp_$(mintload)
realtarget = $(mintloadtarget)

$(mintloadtarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildmintload: $(realtarget)
