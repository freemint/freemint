#
# Makefile for kernel
#

SHELL = /bin/sh
SUBDIRS = arch buildinfo gen-syscall libkern sockets xdd xfs
KERNELSUBDIRS = arch buildinfo

srcdir = .
top_srcdir = .
subdir = 

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-kernels

# default overwrites

# default definitions
GENFILES = $(compile_all_dirs)


help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - sockets / xdd / xfs'
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

all-kernels:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(kerneltargets); do \
		echo "Making $$i"; \
		($(MAKE) $$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(kerneltargets):
	$(MAKE) buildkernel kernel=$@

sockets xdd xfs:
	cd $@; $(MAKE) all
