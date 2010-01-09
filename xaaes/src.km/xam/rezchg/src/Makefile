#
# Makefile for project
#
TARGET = 
MODULES = rcvidel rcnova rcmilan

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../../..
subdir = src

installdir = /tmp/project

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
CFLAGS += -g -mshort
CPU = 000

# default definitions
DEFINITIONS = -D__KERNEL__ -D__KERNEL_MODULE__ -DMODULE_NAME=tEst
INCLUDES += -I$(top_srcdir)/src.km
INCLUDES += -I$(top_srcdir)/../../sys
LIBS += -lc -lgcc16

SGENFILES = *.xam

OBJS = $(COBJS:.c=.o) $(SOBJS:.s=.o)

RCVIDEL_OBJS = $(RCVIDEL_SRCS:.c=.o)
RCNOVA_OBJS = $(RCNOVA_SRCS:.c=.o)
RCMILAN_OBJS = $(RCMILAN_SRCS:.c=.o)

GENFILES = $(TARGET)
LD = $(CC) -nostdlib -Wl,--entry -Wl,_init

$(OBJS): Makefile

all: $(MODULES)

$(TARGET): $(XOBJS)
	$(LD) -o $@ $(CFLAGS) $(LDFLAGS) $(XOBJS) $(LIBS)

rcvidel:
	$(MAKE) rcvidel.xam \
		"TARGET=rcvidel.xam" \
		"XOBJS=$(RCVIDEL_OBJS)"
		
rcnova:
	$(MAKE) rcnova.xam \
		"TARGET=rcnova.xam" \
		"XOBJS=$(RCNOVA_OBJS)"

rcmilan:
	$(MAKE) rcmilan.xam \
		"TARGET=rcmilan.xam" \
		"XOBJS=$(RCMILAN_OBJS)"

strip:
	$(STRIP) $(TARGET)
	
install: all
	$(top_srcdir)/mkinstalldirs $(installdir)
	cp $(TARGET) $(srcdir)/qed.rsc $(srcdir)/qed.hrd $(installdir)
	chmod 755 $(installdir)/$(TARGET)
	$(STRIP) $(installdir)/$(TARGET)

include $(top_srcdir)/DEPENDENCIES
