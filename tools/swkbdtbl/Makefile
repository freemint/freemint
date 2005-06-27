#
# the keyboard table switcher
#
TARGET = swkbdtbl

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
LIBS = 

# default definitions
OBJS = $(TARGET).o
GENFILES = $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

# default dependencies
# must be included last
include $(top_srcdir)/DEPENDENCIES
