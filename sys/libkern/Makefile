#
# Makefile for libkern.a
#

SHELL = /bin/sh
SUBDIRS = asm

srcdir = .
top_srcdir = ..
subdir = libkern

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-libs

# default overwrites

# default definitions
GENFILES = .cpu_* libkern*.a


help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - $(cputargets)'
	@echo '#'
	@echo '# - bakclean'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '#'
	@echo '# example for a 030 libkern: -> make 030'
	@echo '#'

all-libs:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(cputargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(cputargets):
	$(MAKE) libkern$@.a CPU=$@

libkern$(CPU).a: objs asm-files
	$(shell \
	  if [ ! -e $@ \
	         -o .cpu_$(CPU)/objs -nt $@ \
	         -o asm/.cpu_$(CPU)/asm-files -nt $@ ] ; \
	  then \
		objs=`cat .cpu_$(CPU)/objs` ; \
		objs=`for i in $$objs ; do echo .cpu_$(CPU)/$$i ; done` ; \
		\
		asmobjs=`cat asm/.cpu_$(CPU)/asm-files` ; \
		asmobjs=`for i in $$asmobjs ;  do echo asm/.cpu_$(CPU)/$$i ; done` ; \
		\
		echo "$(RM) $@; $(AR) rc $@ $$objs $$asmobjs; $(RANLIB) $@;" ; \
	  fi \
	)

objs: .cpu_$(CPU)
	cd .cpu_$(CPU); $(MAKE)

.cpu_$(CPU): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile

asm-files:
	cd asm; $(MAKE) CPU="$(CPU)" asm-files
