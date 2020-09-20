#
# the keyboard table translator program
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = mktbl

default: help

include $(srcdir)/MKTBLDEFS

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
	@echo '# - $(mktbltargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

strip:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(mktbltargets); do \
		(set -x; \
		($(STRIP) .compile_$$i/mktbl) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac); \
	done && test -z "$$fail"

all-targets:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(mktbltargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(mktbltargets):
	$(MAKE) buildmktbl mktbl=$@

#
# multi target stuff
#

ifneq ($(mktbl),)

compile_dir = .compile_$(mktbl)
mktbltarget = _stmp_$(mktbl)
realtarget = $(mktbltarget)

$(mktbltarget): $(compile_dir)
	cd $(compile_dir); $(MAKE) all

$(compile_dir): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

else

realtarget =

endif

buildmktbl: $(realtarget)

TBLDIR = ../../sys/tbl

compile-tables:
	for dir in Britain Czech Netherlands Finland France Germany Hungary Norway Italy Russian Spain Sweden Switzerland USA Poland; do \
		mkdir -p $(TBLDIR)/$$dir || exit 1; \
	done
	$(NATIVECC) $(NATIVECFLAGS) -DMAIN -o mktbl mktbl.c unitable.c
	./mktbl keyboards/british.src $(TBLDIR)/Britain/british.tbl
	./mktbl keyboards/czech_iso.src $(TBLDIR)/Czech/czech_iso.tbl
	./mktbl keyboards/czech_kam.src $(TBLDIR)/Czech/czech_kam.tbl
	./mktbl keyboards/dutch.src $(TBLDIR)/Netherlands/dutch.tbl
	./mktbl keyboards/finnish.src $(TBLDIR)/Finland/finnish.tbl
	./mktbl keyboards/french.src $(TBLDIR)/France/french.tbl
	./mktbl keyboards/french-pc.src $(TBLDIR)/France/french-pc.tbl
	./mktbl keyboards/german.src $(TBLDIR)/Germany/german.tbl
	./mktbl keyboards/hungarian.src $(TBLDIR)/Hungary/hungarian.tbl
	./mktbl keyboards/norwegian.src $(TBLDIR)/Norway/norwegian.tbl
	./mktbl keyboards/italian.src $(TBLDIR)/Italy/italian.tbl
	./mktbl keyboards/russian.src $(TBLDIR)/Russian/russian.tbl
	./mktbl keyboards/spanish.src $(TBLDIR)/Spain/spanish.tbl
	./mktbl keyboards/swedish.src $(TBLDIR)/Sweden/swedish.tbl
	./mktbl keyboards/swiss_german.src $(TBLDIR)/Switzerland/swiss_de.tbl
	./mktbl keyboards/swiss_french.src $(TBLDIR)/Switzerland/swiss_fr.tbl
	./mktbl keyboards/uk2us.src $(TBLDIR)/USA/uk2us.tbl
	./mktbl keyboards/british-pl.src $(TBLDIR)/Poland/british.tbl
	./mktbl keyboards/french-pl.src $(TBLDIR)/Poland/french.tbl
	./mktbl keyboards/german-pl.src $(TBLDIR)/Poland/german.tbl
	./mktbl keyboards/italian-pl.src $(TBLDIR)/Poland/italian.tbl
	./mktbl keyboards/spanish-pl.src $(TBLDIR)/Poland/spanish.tbl
	./mktbl keyboards/swiss_german-pl.src $(TBLDIR)/Poland/swiss_de.tbl
	./mktbl keyboards/swiss_french-pl.src $(TBLDIR)/Poland/swiss_fr.tbl
	./mktbl keyboards/usa-pl.src $(TBLDIR)/Poland/usa.tbl
	$(RM) mktbl
