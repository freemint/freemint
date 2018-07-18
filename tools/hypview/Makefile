#
# Makefile for hyp_view
#
TARGET = hyp_view.app

SHELL = /bin/sh
SUBDIRS = hyp plain dragdrop bubble

srcdir = .
top_srcdir = ..
subdir = hypview

installdir = /opt/GEM/hypview

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

ifeq (v4e,$(CPU))
CFLAGS += -mcpu=5474
endif

ifeq (yes,$(LIBCMINI))
INCLUDES := -I$(LIBCMINI_PATH)/include $(INCLUDES)
endif

all-here: $(TARGET)

# default overwrites
CFLAGS += -D_GNU_SOURCE
CFLAGS += -g

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -Lplain -lplain -Lhyp -lhyp -Ldragdrop -ldgdp -lgem
GENFILES = $(TARGET) pc.pdb

ifneq (yes,$(LIBCMINI))
$(TARGET): $(OBJS) hyp/libhyp.a plain/libplain.a dragdrop/libdgdp.a bubble/libbgh.a
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -Wl,-stack,128k
	$(STRIP) $(TARGET)
else
$(TARGET): $(OBJS) hyp/libhyp.a plain/libplain.a dragdrop/libdgdp.a bubble/libbgh.a
	$(CC) -I$(LIBCMINI_PATH)/include -nostdlib -o $@ $(CFLAGS) $(LIBCMINI_STARTUP) $(LDFLAGS) $(OBJS) -L$(LIBCMINI_LIBPATH) $(LIBS) -lcmini -lgcc -Wl,-stack,128k
	$(STRIP) $(TARGET)
endif

hyp/libhyp.a plain/libplain.a dragdrop/libdgdp.a bubble/libbgh.a: all-recursive

include $(top_srcdir)/DEPENDENCIES

install: all
	$(top_srcdir)/mkinstalldirs $(installdir)
	cp $(TARGET) $(srcdir)/hyp_view.rsc $(srcdir)/hyp_view.hrd $(installdir)
	chmod 755 $(installdir)/$(TARGET)
	$(STRIP) $(installdir)/$(TARGET)
