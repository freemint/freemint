#
# Makefile for eth
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../../..
subdir = eth

default:

include ./ETHDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY


all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.udd *.prg

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(ethtargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(ethtargets): 
	$(MAKE) buildeth eth=$@

#
# multi target stuff
#

ifneq ($(eth),)

compile_dir = .compile_$(eth)
ethtarget = _stmp_$(eth)
realtarget = $(ethtarget)

$(ethtarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildeth: $(realtarget)
