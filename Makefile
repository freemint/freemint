#
# Makefile for freemint
#

SHELL = /bin/sh
SUBDIRS = sys tools xaaes

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
