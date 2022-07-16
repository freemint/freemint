#
# Makefile for freemint
#

SHELL = /bin/sh
SUBDIRS = shared sys xaaes tools

srcdir = .
top_srcdir = .
subdir = 

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: 

# default overwrites

# default definitions

# kernel targets (repeated here from sys/KERNELDEFS)
kerneltargets = 000 020 030 040 060 deb mil ara hat hat030
ifeq ($(COLDFIRE),yes)
kerneltargets += col
endif

000 020 030 040 060 deb mil col ara hat hat030 sockets xdd xfs usb all-kernels::
	$(MAKE) -C sys $@

000 020 030 040 060 deb mil col ara hat hat030 all-kernels::
	$(MAKE) -C xaaes $@

all-recursive::
	$(MAKE) -C doc/xaaes

clean::
	$(MAKE) -C doc/xaaes $@

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - sockets / xdd / xfs / usb'
	@echo '# - all-kernels'
	@echo '# - $(kerneltargets)'
	@echo '#'
	@echo '# - bakclean'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '#'
	@echo '# example for a Milan kernel: -> make mil'
	@echo '#'
