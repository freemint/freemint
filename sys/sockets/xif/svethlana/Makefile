#
# Makefile for
#
TARGET = svethlan.xif

GENFILES = $(TARGET)

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../../..
subdir = svethlana

default: all

CPU = 060

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

strip:
	$(STRIP) $(TARGET)

# default overwrites
INCLUDES = -I../.. -I$(top_srcdir)
XIF_DEFINITIONS =
DEFINITIONS = -D__KERNEL_XDD__ $(XIF_DEFINITIONS)

LD = $(CC) -mshort -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc

# default definitions
OBJS = $(addsuffix .o, $(basename $(notdir $(SVETHLANA_SRCS) $(COMMON_SRCS))))

main.o:	../main.c
	$(CC) -o $@ $(CFLAGS) -c ../main.c

# ethernat driver target
$(TARGET): $(OBJS) $(LIBKERNSTMP)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

include $(top_srcdir)/DEPENDENCIES
