#
# Makefile for gluestik, a stik emulator for mintnet
#
TARGET = gluestik.prg

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = gluestik

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
GENERAL = -mshort

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -lsocket16
GENFILES = $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)
	$(STRIP) $@
	$(FLAGS) --mglobal-memory $@


include $(top_srcdir)/DEPENDENCIES
