#
# Makefile for minixfs and friends
#

TARGETDIR = ../bin

MINTINCLUDE = /home/fnaumann/mint-1.15/src/include

CC = gcc -pipe
AS = $(CC) -c
CFLAGS = \
	-I$(MINTINCLUDE) \
	-Wall -mshort -I. \
	-O2 -fomit-frame-pointer

#
# top level
#
all: $(TARGETDIR)/flist $(TARGETDIR)/mfsconf

$(TARGETDIR)/flist: flist.c
	$(CC) -o $@ $(CFLAGS) flist.c

$(TARGETDIR)/mfsconf: mfsconf.c
	$(CC) -o $@ $(CFLAGS) mfsconf.c

clean:
	rm -f *.o


distclean:
	rm -f *.o
	rm -f *~
