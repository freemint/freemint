#
# the m68k arch specific files
#

SHELL = /bin/sh
SUBDIRS = asmtrans magic

srcdir = .
top_srcdir = ..
subdir = arch

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: buildkernel

# default overwrites

# default definitions
GENFILES = $(compile_all_dirs)
