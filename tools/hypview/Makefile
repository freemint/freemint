#
# Makefile for hyp_view
#

SHELL = /bin/sh
SUBDIRS = hyp plain dragdrop bubble

srcdir = .
top_srcdir = ..
subdir = hypview

installdir = /opt/GEM/hypview
cpu = all

default: help

include $(srcdir)/HYPVIEWDEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(hypviewtargets)'
	@echo '#'
	@echo '# - install'
	@echo '# - strip'
	@echo '# - clean'
	@echo '# - bakclean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '#'
	@echo '# example for a ColdFire binary: -> make col'
	@echo '#'
	@echo '# To install single CPU binary: ->'
	@echo '# make install cpu=<CPU> installdir=<DIR>'
	@echo '#'

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(hypviewtargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"


install: $(cpu)
	$(MKDIR) -p $(installdir)
ifeq ($(cpu), all)
	@set -x; \
	for i in $(hypviewtargets); do \
		$(CP) .compile_$$i/hyp_view.app $(installdir)/hv$$i.app; \
		chmod 755 $(installdir)/hv$$i.app; \
		$(STRIP) $(installdir)/hv$$i.app; \
	done
else
	$(CP) .compile_$(cpu)/hyp_view.app $(installdir)/hyp_view.app
	chmod 755 $(installdir)/hyp_view.app
	$(STRIP) $(installdir)/hyp_view.app
endif
	$(CP) -r $(srcdir)/doc $(installdir)
	$(MKDIR) -p $(installdir)/de
	$(CP) -r $(srcdir)/hyp_view/de.rsc $(installdir)/de/hyp_view.rsc
	$(MKDIR) -p $(installdir)/cs
	$(CP) -r $(srcdir)/hyp_view/cs.rsc $(installdir)/cs/hyp_view.rsc
	$(CP) $(srcdir)/release/* $(installdir)
	$(CP) -r $(srcdir)/skins $(installdir)
	$(CP) $(srcdir)/hyp_view.bgh $(installdir)
	$(CP) $(srcdir)/hyp_view/en.rsc $(installdir)/hyp_view.rsc

$(hypviewtargets):
	$(MAKE) buildhypview hypview=$@

#
# multi target stuff
#

ifneq ($(hypview),)

compile_dir = .compile_$(hypview)
hypviewtarget = _stmp_$(hypview)
realtarget = $(hypviewtarget)

$(hypviewtarget): $(compile_dir)
	@set fnord $(MAKEFLAGS); amf=$$2; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$hypview in $$subdir"; \
	  (cd $$subdir && $(MAKE) -r $$hypview) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildhypview: $(realtarget)
