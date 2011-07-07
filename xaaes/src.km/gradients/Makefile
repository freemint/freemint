SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = gradients

include $(top_srcdir)/CONFIGVARS

all: g_blue.grd g_cyan.grd g_green.grd g_purple.grd g_red.grd g_yellow.grd default.grd

%.grd: %.c
	$(CC) -c -o $(@:.c=.grd) $<
