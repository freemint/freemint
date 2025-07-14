#
# Makefile for the syscall parsing library
#
TARGET = libsyscall.a

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = syscall

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
CC = $(NATIVECC)
CFLAGS = $(NATIVECFLAGS)
AR = ar
RANLIB = ranlib

# default definitions
OBJS = $(COBJS:.c=.o)
OBJS += pars.o scan.o
GENFILES = $(TARGET)
GENFILES += pars.c pars.h scan.c .gen_parse

syscallparser.o: syscallparser.c
	$(CC) $(CFLAGS) -c $< -o $@
pars.o: .gen_parse
	$(CC) $(CFLAGS) -c pars.c -o $@
scan.o: scan.c .gen_parse
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(AR) rcs $@.$$$$ $(OBJS) && mv $@.$$$$ $@ || { $(RM) $@.$$$$; false; }

.gen_parse: pars.y
	$(YACC) --output=pars.c pars.y
	@touch $@

scan.c: scan.l
	$(LEX) -o$@ scan.l
