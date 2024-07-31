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

all-here: libsyscall $(TARGET)

# default overwrites
NATIVECFLAGS += -D_GNU_SOURCE
NATIVECFLAGS += -I$(top_srcdir)/../shared
NATIVE_LIBS_DEP += $(top_srcdir)/../shared/libsyscall/libsyscall.a
NATIVE_LIBS += -L$(top_srcdir)/../shared/libsyscall -lsyscall
#NATIVE_LIBS += -lfl

MASTER = $(top_srcdir)/../sys/syscalls.master

CC = $(NATIVECC)
CFLAGS = $(NATIVECFLAGS)
LIBS = $(NATIVE_LIBS)
LIBS_DEP = $(NATIVE_LIBS_DEP)

# default definitions
OBJS = gentables.c
GENFILES = $(TARGET)

libsyscall:
	cd $(top_srcdir)/../shared/libsyscall; $(MAKE)

$(TARGET): $(OBJS) $(LIBS_DEP)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
