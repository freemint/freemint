SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = gradients

include $(top_srcdir)/CONFIGVARS

all: g_blue.grd g_cyan.grd g_green.grd g_purple.grd g_red.grd g_yellow.grd default.grd \
	n_blue.grd n_cyan.grd n_green.grd n_purple.grd n_red.grd n_yellow.grd n_default.grd

%.grd: %.c
	$(CC) -c -o $(@:.c=.grd) $<
