#
# Makefile for TosWin 2
#

SHELL = /bin/sh
SUBDIRS = tw-call

srcdir = .
top_srcdir = ..
subdir = toswin2

installdir = /opt/GEM/toswin2
cpu = all

default: help

include $(srcdir)/TOSWIN2DEFS

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets doc/toswin2.hyp

# default overwrites

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(toswin2targets)'
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

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(toswin2targets); do \
		(set -x; \
		($(STRIP) .compile_$$i/*.app) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(toswin2targets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

doc/toswin2.hyp: doc/toswin2.stg
	@if test "$(HCP)" != ""; then hcp="$(HCP)"; else hcp=hcp; fi; \
	if test "$$hcp" != "" && $$hcp --version >/dev/null 2>&1; then \
		$$hcp -o $@ $<; \
	else \
		echo "HCP not found, help file not compiled" >&2; \
	fi

install: $(cpu) doc/toswin2.hyp
	$(MKDIR) -p $(installdir)
ifeq ($(cpu), all)
	@set -x; \
	for i in $(toswin2targets); do \
		$(CP) .compile_$$i/toswin2.app $(installdir)/tw2$$i.app; \
		$(CP) tw-call/.compile_$$i/tw-call.app $(installdir)/twc$$i.app; \
		chmod 755 $(installdir)/tw2$$i.app $(installdir)/twc$$i.app; \
		$(STRIP) $(installdir)/tw2$$i.app $(installdir)/twc$$i.app; \
	done
else
	$(CP) .compile_$(cpu)/toswin2.app $(installdir)/toswin2.app
	$(CP) tw-call/.compile_$(cpu)/tw-call.app $(installdir)/tw-call.app
	chmod 755 $(installdir)/toswin2.app $(installdir)/tw-call.app
	$(STRIP) $(installdir)/toswin2.app $(installdir)/tw-call.app
endif
	$(CP) -r $(srcdir)/doc $(installdir)
	$(CP) $(srcdir)/english/toswin2.rsc $(installdir)
	$(CP) -r $(srcdir)/screenshots $(installdir)
	$(CP) $(srcdir)/BUGS $(installdir)
	$(CP) $(srcdir)/FAQ $(installdir)
	$(CP) $(srcdir)/NEWS $(installdir)
	$(MKDIR) -p $(installdir)/de
	$(CP) $(srcdir)/toswin2.rsc $(installdir)/de
	$(CP) $(srcdir)/allcolors.sh $(installdir)
	$(CP) $(srcdir)/twterm.src $(installdir)
	$(CP) $(srcdir)/README.terminfo $(installdir)
	$(CP) $(srcdir)/vttest.txt $(installdir)

$(toswin2targets):
	$(MAKE) buildtoswin2 toswin2=$@

#
# multi target stuff
#

ifneq ($(toswin2),)

compile_dir = .compile_$(toswin2)
toswin2target = _stmp_$(toswin2)
realtarget = $(toswin2target)

$(toswin2target): $(compile_dir)
	@set fnord $(MAKEFLAGS); amf=$$2; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$toswin2 in $$subdir"; \
	  (cd $$subdir && $(MAKE) -r $$toswin2) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildtoswin2: $(realtarget)

clean::
	$(RM) doc/toswin2.hyp doc/toswin2.ref
