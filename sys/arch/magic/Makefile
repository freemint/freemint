#
# the m68k genmagic program
#
TARGET = genmagic

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = magic

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: magic.i

# default overwrites
CFLAGS = $(GENERAL) -I$(top_srcdir)
LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc

# default definitions
OBJS = $(COBJS:%.c=%.o) $(SOBJS:%.s=%.o) $(SPPOBJS:%.spp=%.o)
GENFILES = $(TARGET) magic.i
CPU = 000


$(TARGET): $(OBJS) $(LIBKERNSTMP)
	$(LD) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)


ifeq ($(CROSS),yes)

# for cross-compiling use this: (assumes you have perl and cnm)
magic.i: $(TARGET)
	perl genmagic.pl > $@

else

magic.i: $(TARGET)
	./$(TARGET) $@

endif


# default dependencies
# must be included last
include $(top_srcdir)/DEPENDENCIES
