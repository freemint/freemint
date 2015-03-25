#
# Makefile for
#
TARGET = ext2.xfs
BUILD_PREFIX = .compile_
SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

targets=000 020-60 v4e deb

all: $(targets)

include SRCFILES

$(targets): $(SRCFILES)
	mkdir -p $(BUILD_PREFIX)$@ && cd $(BUILD_PREFIX)$@ && make -f ../Makefile.objs CPU=$@

BINFILES: Makefile
	$(SHELL) -c '{ \
	echo "# This file gets included by the Makefile in this directory to determine"; \
	echo "# the files that should go only into binary distributions."; \
	echo; echo -n "BINFILES ="; \
	for t in $(targets); do echo -n " $(BUILD_PREFIX)$$t/$(TARGET)"; done; echo; } >$@'

