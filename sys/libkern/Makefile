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

# all-here: $(shell basename $(LIBKERNTARGET))
all-here: 

# default overwrites

# default definitions
GENFILES = .000 .030 .020-60 libkern000.a libkern030.a libkern020-60.a


libkern$(CPU).a: objs asm-files
	$(RM) $@
	$(AR) rc $@ \
		$(shell for file in `cat .$(CPU)/objs` ; \
			do echo .$(CPU)/$$file ; done) \
		$(shell for file in `cat asm/.$(CPU)/asm-files` ; \
			do echo asm/.$(CPU)/$$file ; done)
	$(RANLIB) $@

objs: .$(CPU)
	cd .$(CPU); $(MAKE)

.$(CPU):
	$(MKDIR) $@
	$(CP) Makefile.objs $@/Makefile

asm-files:
	cd asm; $(MAKE) CPU="$(CPU)" asm-files
