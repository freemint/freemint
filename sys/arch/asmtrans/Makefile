#
# assembler source files are now handled in a radically different
# fashion. We build a pre-processor program, asmtrans, that
# takes the .spp files, merges them with an include file that
# gives various offsets into structures of interest, and produces
# the .s files as output. This has two major advantages:
# (1) it lets us use the same source for both the Lattice and
#     gcc assembler files (the translator will convert), and
# (2) if we change the CONTEXT or PROC structures, we don't
#     have to dig through the source code looking for
#     magic numbers
#
TARGET = asmtrans

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = 

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
CC = $(NATIVECC)
CFLAGS = $(NATIVECFLAGS)
LIBS =
YACC = bison
YTABC = asm.tab.c
YTABH = asm.tab.h

# default definitions
OBJS = asmtab.o trutil.o trans.o
SRC = asm.y asmtab.c asmtab.h asmtrans.h trutil.c trans.c
GENFILES = $(TARGET) asmtab.c asmtab.h

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

asmtab.c asmtab.h: asm.y
	$(YACC) -d asm.y
	mv $(YTABC) asmtab.c
	mv $(YTABH) asmtab.h

# default dependencies
# must be included last
include $(top_srcdir)/DEPENDENCIES
