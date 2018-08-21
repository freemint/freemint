#
# Makefile for gluestik, a stik emulator for mintnet
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = gluestik

default: all-here

include $(srcdir)/GLUESTIKDEFS

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
	for i in $(gluestiktargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(gluestiktargets):
	$(MAKE) buildgluestik gluestik=$@

#
# multi target stuff
#

ifneq ($(gluestik),)

compile_dir = .compile_$(gluestik)
gluestiktarget = _stmp_$(gluestik)
realtarget = $(gluestiktarget)

$(gluestiktarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildgluestik: $(realtarget)
