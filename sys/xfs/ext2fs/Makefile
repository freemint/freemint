#
# Makefile for ext2.xfs
#

TARGET = ../ext2.xfs
TARGET_st = ../ext2_st.xfs

MINTINCLUDE = /home/fnaumann/mint-1.15/src
FSDIR = /c/multitos

# compiler settings
CC = gcc -pipe
LD = $(CC) -nostdlib -mshort -Wl,--entry -Wl,_init
LIBS = $(KERNLIB) -lgcc
CFLAGS = \
	-I$(MINTINCLUDE) -I. -mshort \
	-Wall \
	-Wmissing-prototypes \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-qual \
	-Waggregate-return \
	-fomit-frame-pointer \
	-O2 $(MACHINE)


# used tools
MV = mv
CP = cp
RM = rm -f
TOUCH = touch
MKDIR = mkdir
MAKE = make
MKDEP = $(CC) $(CFLAGS) -MM
SED = sed
CRLF = crlf -s

# generic object dir
OBJDIR = objs

$(OBJDIR)/%.d: %.c
	@echo "# Computing dependencies for $*.o"
	@o='o'; \
	$(MKDEP) $< >$@.tmp \
	  && $(SED) "s,^\(.*\)\.o:,$(OBJDIR)/\1.$$o $@:," < $@.tmp > $@ \
	  && $(RM) $@.tmp

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

COBJS = \
	$(OBJDIR)/balloc.o \
	$(OBJDIR)/bitmap.o \
	$(OBJDIR)/ext2dev.o \
	$(OBJDIR)/ext2sys.o \
	$(OBJDIR)/global.o \
	$(OBJDIR)/ialloc.o \
	$(OBJDIR)/inode.o \
	$(OBJDIR)/main.o \
	$(OBJDIR)/namei.o \
	$(OBJDIR)/super.o \
	$(OBJDIR)/truncate.o

DEPENDENCIES = $(COBJS:.o=.d)
OBJS = $(COBJS)


all:
	make $(TARGET) OBJDIR=objs MACHINE=-m68020-60 KERNLIB=../../../libkern/libkern020-60.a
	make $(TARGET_st) OBJDIR=objs_st MACHINE=-m68000 KERNLIB=../../../libkern/libkern000.a

$(TARGET): $(DEPENDENCIES) $(OBJS)
	$(LD) -o $@ $(OBJS) $(LIBS)
	strip $@

$(TARGET_st): $(DEPENDENCIES) $(OBJS)
	$(LD) -o $@ $(OBJS) $(LIBS)
	strip $@

clean:
	make do_clean OBJDIR=objs
	make do_clean OBJDIR=objs_st

do_clean:
	rm -f $(COBJS)

distclean:
	make do_distclean OBJDIR=objs
	make do_distclean OBJDIR=objs_st
	rm -f *~

do_distclean:
	rm -f $(OBJS)
	rm -f $(DEPENDENCIES)


boot: $(TARGET)
	$(CP) $(TARGET) $(FSDIR)

bootst: $(TARGET_st)
	$(CP) $(TARGET_st) $(FSDIR)


-include $(DEPENDENCIES)
