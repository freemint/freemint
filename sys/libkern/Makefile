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
GENFILES = .compile_* libkern*.a


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
	         -o .compile_$(CPU)/objs -nt $@ \
	         -o asm/.compile_$(CPU)/asm-files -nt $@ ] ; \
	  then \
		objs=`cat .compile_$(CPU)/objs` ; \
		objs=`for i in $$objs ; do echo .compile_$(CPU)/$$i ; done` ; \
		\
		asmobjs=`cat asm/.compile_$(CPU)/asm-files` ; \
		asmobjs=`for i in $$asmobjs ;  do echo asm/.compile_$(CPU)/$$i ; done` ; \
		\
		echo "$(RM) $@; $(AR) rcs $@ $$objs $$asmobjs" ; \
	  fi \
	)

objs: .compile_$(CPU)
	cd .compile_$(CPU); $(MAKE)

# prevent overriding the same recipe from RULES
ifneq ($(compile_dir),.compile_$(CPU))
.compile_$(CPU): Makefile.objs
	$(MKDIR) -p $@
	$(CP) $< $@/Makefile
endif

clean clean-recursive distclean distclean-recursive bakclean bakclean-recursive::
	@list='$(cputargets)'; for dir in $$list; do \
	  subdir=".compile_$${dir}"; \
	  target=`echo $@ | sed s/-recursive//`; \
	  test -d $$subdir && { echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) -r $$target); } || true; \
	  target="libkern$${dir}.a"; \
	done
	$(RM) objs

asm-files:
	cd asm; $(MAKE) CPU="$(CPU)" asm-files
