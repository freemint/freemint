#
# Makefile for NoHog2
#
TARGET = nohog2.acc

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = nohog2

default: all


include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

ifeq (v4e,$(CPU))
CFLAGS += -mcpu=5474
endif

all-here: $(TARGET)

# default overwrites

# default definitions
OBJS = $(COBJS:.c=.o)
LIBS += -lgem
GENFILES = $(TARGET)

ifneq (yes,$(LIBCMINI))
$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)
else
$(TARGET): $(OBJS)
	$(CC) -I$(LIBCMINI_PATH)/include -nostdlib -o $@ $(CFLAGS) $(LIBCMINI_STARTUP) $(OBJS) -L$(LIBCMINI_LIBPATH) -lcmini $(LIBS) -lgcc
	$(STRIP) $@
endif


include $(top_srcdir)/DEPENDENCIES

