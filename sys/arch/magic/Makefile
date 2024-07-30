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
ifeq ($(NATIVE_COLDFIRE),1)
CPU = v4e
else
CPU = 000
endif
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: magic.i

# default overwrites
CFLAGS = -I$(top_srcdir) $(GENERAL)
LD = $(CC) -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc

# default definitions
OBJS = $(COBJS:.c=.o) $(SOBJS:.S=.o)
GENFILES = $(TARGET) magic.i


#
# we may end up here by parallel makes from the top directory for different kernels
$(TARGET): $(OBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) $(LDFLAGS) -o $@.$$$$ $(OBJS) $(LIBS) && mv $@.$$$$ $@ || { $(RM) $@.$$$$; false; }


ifeq ($(CROSS),yes)

# for cross-compiling use this: (assumes you have perl and cnm)
magic.i: genmagic.o
	perl genmagic.pl > $@.$$$$ && mv $@.$$$$ $@ || { $(RM) $@.$$$$; false; }

else

magic.i: $(TARGET)
	./$(TARGET) $@

endif

genmagic.o: genmagic.c
	$(CC) $(CFLAGS_$(CPU)) $(MODEL_$(CPU)) -c $< -o $@

startup.o: startup.S
	$(CC) $(CFLAGS_$(CPU)) $(MODEL_$(CPU)) -c $< -o $@
