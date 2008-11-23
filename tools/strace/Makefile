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

all-here: generator $(TARGET)

# default overwrites
NATIVECFLAGS = -I$(top_srcdir)/../shared
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


$(TARGET): genstamp $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

sysenttab.c sysenttab.h: genstamp

genstamp: $(MASTER) gentables
	./gentables $<
	touch $@

generator:
	cd $(top_srcdir)/../shared/libsyscall; $(MAKE)
	$(MAKE) gentables \
		CC="$(NATIVECC)" CFLAGS="$(NATIVECFLAGS)" \
		LIBS_DEP="$(NATIVE_LIBS_DEP)" LIBS="$(NATIVE_LIBS)"

gentables: gentables.o $(LIBS_DEP)
	$(CC) -o $@ $(CFLAGS) $< $(LIBS)


include $(top_srcdir)/DEPENDENCIES
