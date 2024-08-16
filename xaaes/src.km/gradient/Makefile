SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = gradients

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY


GRADIENTS =  \
	autumn.grd \
	default.grd \
	flwrpwr.grd \
	g_blue.grd \
	g_cyan.grd \
	g_green.grd \
	g_purple.grd \
	g_red.grd \
	g_yellow.grd \
	menubar.grd \
	mint.grd \
	n_blue.grd \
	n_cyan.grd \
	n_green.grd \
	n_purple.grd \
	n_red.grd \
	n_yellow.grd \
	n_deflt.grd

all-here: $(GRADIENTS)

# default definitions
GENFILES = *.grd

#
# MiNT must be compiled with 16 bit integers (-mshort)
#
%.grd: %.c
	$(CC) -DWITH_GRADIENTS=1 -mshort -nostdlib -nostartfiles -Wl,--oformat,binary -Wl,-entry,_start -o $@ $<
