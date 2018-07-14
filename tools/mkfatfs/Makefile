#
# Makefile for mkfatfs and gemkfatfs
#
TARGET = mkfatfs gemkfatfs.app

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = mkfatfs

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)/IO

# default definitions
OBJS = $(COBJS:.c=.o) $(SOBJS:.S=.o)
GENFILES = $(TARGET)
CFLAGS += -Wno-inline
LDFLAGS += -s

MKFATFSOBJS = mkfatfs.o
GEMKFATFSOBJS = startup.o gemkfatfs.o util.o

mkfatfs: $(MKFATFSOBJS) $(LIBIOSTMP)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(MKFATFSOBJS) $(LIBIO)

gemkfatfs.app: $(GEMKFATFSOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(GEMKFATFSOBJS) \
		-nostartfiles -nostdlib -lgemma -lc -lgcc


include $(top_srcdir)/DEPENDENCIES
