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
	$(RM) $@
	$(AR) rc $@ \
		$(shell for file in `cat .cpu_$(CPU)/objs` ; \
			do echo .cpu_$(CPU)/$$file ; done) \
		$(shell for file in `cat asm/.cpu_$(CPU)/asm-files` ; \
			do echo asm/.cpu_$(CPU)/$$file ; done)
	$(RANLIB) $@

objs: .cpu_$(CPU)
	cd .cpu_$(CPU); $(MAKE)

.cpu_$(CPU):
	$(MKDIR) $@
	$(CP) Makefile.objs $@/Makefile

asm-files:
	cd asm; $(MAKE) CPU="$(CPU)" asm-files
