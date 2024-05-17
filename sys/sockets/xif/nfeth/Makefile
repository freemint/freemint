#
# Makefile for 
#
TARGET = nfeth.xif

GENFILES = $(TARGET)

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../..
subdir = nfeth

default: all

CPU = 020-60

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
OBJS = $(addsuffix .o, $(basename $(notdir $(HOSTETH_SRCS) $(COMMON_SRCS))))

main.o:	../main.c
	$(CC) -o $@ $(CFLAGS) -c ../main.c

# generic driver target
$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -s -o $@ $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
