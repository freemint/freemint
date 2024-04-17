#
# the m68k 060sp
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = $(compile_dir)

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: objs 060sp.prg

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = $(KERNELDEFAULTDEFS) $(KERNELDEFS)

CFLAGS-mint060sp.S = -m68060

cflags = $(kernel_cflags)
nocflags = $(kernel_nocflags)

# default definitions
OBJS = $(COBJS:%.c=%.o) $(SOBJS:%.S=%.o)

VPATH = ..

objs: $(OBJS)
	echo $(OBJS) > $@

060sp.prg: $(OBJS)
	$(CC) -o $@ $(OBJS) -nostdlib -Wl,--entry -Wl,_init -lgcc

mint060sp.S: fpsp.S isp.S

fpsp.S: motorola/fpsp.sa
	$(SHELL) asm2gas $< > $@

isp.S: motorola/isp.sa
	$(SHELL) asm2gas $< > $@

# default dependencies
# must be included last
include $(top_srcdir)/DEPENDENCIES
