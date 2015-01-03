#
# Makefile for
#
TARGET = ext2.xfs

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

targets=000 020-60 v4e dbg

all: $(targets)

include SRCFILES

$(targets): $(SRCFILES)
	mkdir -p .$@ && cd .$@ && make -f ../Makefile.objs CPU=$@ $(TARGETS)

#include $(top_srcdir)/DEPENDENCIES

