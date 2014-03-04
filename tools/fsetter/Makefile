#
# Makefile for fsetter
#
TARGET = fsetter.app

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = fsetter

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)/IO
INCLUDES += -I/usr/GEM/include

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += $(LIBIO)
LIBS += -L/usr/GEM/lib -lcflib -lgem
GENFILES = $(TARGET)

$(TARGET): $(OBJS) $(LIBIOSTMP)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
