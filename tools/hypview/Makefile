#
# Makefile for hyp_view
#
TARGET = hyp_view.app

SHELL = /bin/sh
SUBDIRS = hyp plain dragdrop

srcdir = .
top_srcdir = ..
subdir = hypview

installdir = /opt/GEM/hypview

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES += -I/usr/GEM/include
CFLAGS += -D_GNU_SOURCE
#CFLAGS += -DDEBUG
CFLAGS += -g

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -L/usr/GEM/lib
LIBS += -Lplain -lplain -Lhyp -lhyp -Ldragdrop -ldgdp -liio -lgem
GENFILES = $(TARGET)

$(TARGET): $(OBJS) hyp/libhyp.a plain/libplain.a
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)
	$(STRIP) $(TARGET)
	$(STACK) -S128K $(TARGET)

include $(top_srcdir)/DEPENDENCIES

install: all
	$(top_srcdir)/mkinstalldirs $(installdir)
	cp $(TARGET) $(srcdir)/hyp_view.rsc $(srcdir)/hyp_view.hrd $(installdir)
	chmod 755 $(installdir)/$(TARGET)
	$(STRIP) $(installdir)/$(TARGET)
