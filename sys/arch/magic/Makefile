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
CFLAGS = -I$(top_srcdir) $(GENERAL)
LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc

# default definitions
OBJS = $(COBJS:%.c=%.o) $(SOBJS:%.s=%.o) $(SPPOBJS:%.spp=%.o)
GENFILES = $(TARGET) magic.i
COLDFIRE := $(shell grep -q V4e /kern/cpuinfo; \
					status=$$?; \
					if [ $$status = 0 ]; \
					then echo 1; \
					else echo 0; \
					fi)
ifeq ($(COLDFIRE),1)
CPU = v4e
else
CPU = 000
endif


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
