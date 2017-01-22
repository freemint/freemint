#
# Makefile for NoHog2
#
TARGET = nohog2.acc

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = nohog2

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -lgem
GENFILES = $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)


include $(top_srcdir)/DEPENDENCIES
