#
# Makefile for gluestik, a stik emulator for magicnet/mintnet
#

srcdir = .
top_srcdir = ..
subdir = socklib

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

INCLUDES = -I../../sys/sockets

PROGRAMS = libsock.a

OBJS = $(COBJS:.c=.o)

all: $(PROGRAMS)

libsock.a: $(OBJS)
	$(RM) $@
	$(AR) rcs $@ $^

$(OBJS): stsocket.h ../../sys/sockets/mintsock.h

clean::
	rm -f *.o *.a *.pdb *.prg *.ttp *.map $(PROGRAMS)
