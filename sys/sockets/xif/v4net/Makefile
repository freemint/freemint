#
# Makefile for
#
TARGET = v4net.xif

GENFILES = $(TARGET)

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../../..
subdir = v4net

default: all

CPU = 000

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

LD = $(CC) -mshort -nostdlib -Wl,--entry,init -Wl,--fatal-warnings
LIBS = $(LIBKERN) -lgcc

# default definitions
OBJS = $(addsuffix .o, $(basename $(notdir $(V4NET_SRCS) $(COMMON_SRCS))))

main.o:	../main.c
	$(CC) -o $@ $(CFLAGS) -c ../main.c

# v4net driver target
$(TARGET): $(OBJS) $(LIBKERNSTMP)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

include $(top_srcdir)/DEPENDENCIES
