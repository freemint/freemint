#
# Makefile using the GCC
#

MKFATFS = mkfatfs

# cross compiling
#CROSS = yes
CROSS = no

#
# target and host compiler (GCC recommended)
# 
ifeq ($(CROSS),yes)
CC = /scratch/fnaumann/bin/m68k-atari-mint-gcc
NATIVECC = gcc
NATIVECFLAGS = -O
MINTLIBINC = -I/scratch/fnaumann/m68k-atari-mint/include
else
CC = gcc -pipe
NATIVECC = $(CC)
NATIVECFLAGS = -O
MINTLIBINC =
endif

AS = $(CC)
LIBS = 

# used tools
MV = mv
CP = cp
RM = rm -f
TOUCH = touch
MKDIR = mkdir
MAKE = make
MKDEP = $(CC) -MM
SED = sed
CRLF = crlf -s

OPTS =	-fomit-frame-pointer -O3

WARN =	-Wall \
	-Wmissing-prototypes \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-qual \
	-Waggregate-return

CFLAGS = -mshort -g $(WARN) $(OPTS)
ASFLAGS = 


# generic object dir
OBJDIR = objs

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.d: %.c
	@echo "# Computing dependencies for $*.o"
	@o='o'; \
	$(MKDEP) $(CFLAGS) $< >$@.tmp \
	  && $(SED) "s,^\(.*\)\.o:,$(OBJDIR)/\1.$$o $@:," < $@.tmp > $@ \
	  && $(RM) $@.tmp


#
# files
#

CFILES = \
	xhdi.c \
	mkfatfs.c

OBJS =	\
	$(OBJDIR)/xhdi.o \
	$(OBJDIR)/mkfatfs.o

DEPENDENCIES = $(OBJS:.o=.d)


#
# main target
#
$(MKFATFS): $(DEPENDENCIES) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


#
# macros for cleaning up
#

clean:
	$(RM) $(OBJS)

distclean:
	$(RM) $(OBJS)
	$(RM) $(DEPENDENCIES)
	$(RM) *~ core


#
# for 'make depend'
#
depend: $(DEPENDENCIES)
	@echo "# Dependencies ok."


#
# dependencies
#

-include $(DEPENDENCIES)
