#
# Makefile for mgw, a draconis TCP/IP emulator for mintnet
#
TARGET = mgw.prg

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = mgw

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

# Uncomment the following line, if you want to
# create a debug version. The debug version dumbs
# everything interesting to stdout.
# If you comment/uncomment this line build the
# version with
# 
#	make clean; make
#
#CFLAGS += -DSHOWDEBUG

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)
	$(STRIP) $@
	$(FLAGS) --mglobal-memory $@

boot:
	$(CP) $(TARGET) /c/multitos

mintboot:
	$(CP) $(TARGET) /c/mint

include $(top_srcdir)/DEPENDENCIES

