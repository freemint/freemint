#
# the syscall generator program
#
TARGET = gen-syscall

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = gen-syscall

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
CC = $(NATIVECC)
CFLAGS = $(NATIVECFLAGS)
LIBS = -lfl

# default definitions
OBJS = check.o generate.o list.o main.o pars.o scan.o
GENFILES = pars.h pars.c scan.c $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

pars.c pars.h: pars.y
	$(YACC) --output=pars.c pars.y

scan.c: scan.l pars.y
	$(LEX) -oscan.c scan.l

# default dependencies
# must be included last
include $(top_srcdir)/DEPENDENCIES
