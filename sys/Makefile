#
# Makefile for kernel
#

SHELL = /bin/sh
SUBDIRS = arch buildinfo gen-syscall libkern

srcdir = .
top_srcdir = .
subdir = 

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: kernel

# default overwrites

# default definitions
GENFILES = $(compile_all_dirs)


help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - $(kerneltargets)'
	@echo '# - allkernels'
	@echo '# - bakclean'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - help'
	@echo '#'
	@echo '# example for a Milan kernel: -> make mil'
	@echo '#'

allkernels:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	for i in $(kerneltargets); do \
		echo "Making kernel=$$i"; \
		($(MAKE) kernel=$$i) \
		|| case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

$(kerneltargets):
	$(MAKE) kernel=$@
