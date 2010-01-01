#
# Makefile for 
#
TARGET = libinet4.a

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = 

NOCFLAGS-tcputil.c = -O2 -O3
CFLAGS-tcputil.c = -O1
NOCFLAGS-udp.c = -O2 -O3
CFLAGS-udp.c = -O1

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I.. -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XDD__

CPU = 000

# default definitions
OBJS = $(COBJS:.c=.o)

$(TARGET): $(OBJS)
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@


include $(top_srcdir)/DEPENDENCIES
