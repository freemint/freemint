#
# Makefile for crypt I/O initialization
#
TARGET = crypto

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = crypto

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)/IO

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += $(LIBIO)
GENFILES = $(TARGET)

$(TARGET): $(OBJS) $(LIBIOSTMP)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
