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
GENFILES += pars.c pars.h scan.c

syscallparser.o: syscallparser.c
	$(CC) $(CFLAGS) -c $< -o $@
pars.o: pars.c
	$(CC) $(CFLAGS) -c $< -o $@
scan.o: scan.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(AR) rcs $@.$$$$ $(OBJS) && mv $@.$$$$ $@ || { $(RM) $@.$$$$; false; }

pars.c pars.h: pars.y
	$(YACC) --output=pars.c pars.y

scan.c: scan.l pars.y
	$(LEX) -o$@ scan.l
