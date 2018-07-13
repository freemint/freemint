#
# Makefile for the strace system tool
#
TARGET = strace

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = strace

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: gentables $(TARGET)

# default overwrites
NATIVECFLAGS += -I$(top_srcdir)/../shared
NATIVE_LIBS_DEP += $(top_srcdir)/../shared/libsyscall/libsyscall.a
NATIVE_LIBS += -L$(top_srcdir)/../shared/libsyscall -lsyscall
NATIVE_LIBS += -lfl

MASTER = $(top_srcdir)/../sys/syscalls.master
DEFINITIONS += -D_GNU_SOURCE

NOCFLAGS-print.c = -Wcast-qual

# default definitions
OBJS = $(COBJS:.c=.o)
OBJS += sysenttab.o
GENFILES = $(TARGET) sysenttab.* gentables genstamp

sysenttab.o: sysenttab.h

$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

sysenttab.c sysenttab.h: genstamp
strace.o: sysenttab.h

genstamp: $(MASTER) gentables
	./gentables $<
	touch $@

gentables.o: gentables.c $(top_srcdir)/../shared/libsyscall/syscalldefs.h
	$(NATIVECC) $(NATIVECFLAGS) -c $< -o $@

gentables: gentables.o $(NATIVE_LIBS_DEP)
	$(NATIVECC) -o $@ $(NATIVECFLAGS) $< $(NATIVE_LIBS)

$(top_srcdir)/../shared/libsyscall/libsyscall.a:
	$(MAKE) -C $(top_srcdir)/../shared/libsyscall

include $(top_srcdir)/DEPENDENCIES
