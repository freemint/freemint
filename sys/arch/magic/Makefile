#
# the m68k genmagic program
#
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
INCLUDES = -I$(top_srcdir)
LDEXTRA = -nostdlib -Wl,--entry,_init
LIBS = -lgcc

# default definitions
GENFILES = genmagic magic.i


#
# Note: we may end up here by parallel makes from the top directory for different kernels
#

ifeq ($(CROSS),yes)

# for cross-compiling use this: (assumes you have perl and cnm)
magic.i: genmagic.c
	$(CC) $(CFLAGS_$(CPU)) -c $< -o genmagic.$$$$.o && perl genmagic.pl genmagic.$$$$.o > $@.$$$$ && { $(RM) genmagic.$$$$.o; mv $@.$$$$ $@; } || { $(RM) $@.$$$$; genmagic.$$$$.o; false; }

else

OBJS = $(foreach OBJ, $(notdir $(basename $(COBJS))), $(OBJ).o)

genmagic: $(OBJS)
	$(LD) $(CFLAGS_$(CPU)) -o $@.$$$$ $(OBJS) $(LIBS) && mv $@.$$$$ $@ || { $(RM) $@.$$$$; false; }


magic.i: genmagic
	./genmagic $@

genmagic.o: genmagic.c
	$(CC) $(CFLAGS_$(CPU)) -c $< -o $@

startup.o: startup.S
	$(CC) $(CFLAGS_$(CPU)) -c $< -o $@

endif
