#
# Makefile for libkern.a
#

SHELL = /bin/sh
SUBDIRS = asm

srcdir = .
top_srcdir = ..
subdir = libkern

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: 

# default overwrites

# default definitions
GENFILES = .cpu_* libkern*.a


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

.cpu_$(CPU):
	$(MKDIR) $@
	$(CP) Makefile.objs $@/Makefile

asm-files:
	cd asm; $(MAKE) CPU="$(CPU)" asm-files
