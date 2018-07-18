#
# Makefile for hyp_view (hyp)
#
TARGET = libhyp.a

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = hyp

installdir =

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
CFLAGS += -g

ifneq (yes,$(LIBCMINI))
INCLUDES = -I$(top_srcdir)/hypview
else
INCLUDES = -I$(LIBCMINI_PATH)/include -I$(top_srcdir)/hypview
endif

# default definitions
OBJS = $(COBJS:.c=.o)

$(TARGET) : $(OBJS)
	$(AR) cr $@ $(OBJS)
	$(RANLIB) $@

include $(top_srcdir)/DEPENDENCIES
