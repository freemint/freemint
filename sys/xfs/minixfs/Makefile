#
# Makefile using the GCC
#

# cross compiling
#CROSS = yes
CROSS = no

#
# target and host compiler (GCC recommended)
# 
ifeq ($(CROSS),yes)
CC = /scratch_hp/fnaumann/bin/m68k-atari-mint-gcc
NATIVECC = gcc
NATIVECFLAGS = -O
MINTLIBINC = -I/scratch_hp/fnaumann/m68k-atari-mint/include
MINTINCLUDE = -I/scratch_hp/fnaumann/m68k-atari-mint/include
STRIP = /scratch_hp/fnaumann/m68k-atari-mint/bin/xstrip
else
CC = gcc -pipe
NATIVECC = $(CC)
NATIVECFLAGS = -O
MINTLIBINC =
MINTINCLUDE = -I/home/fnaumann/mint-1.15/src/include
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

OPTS =	-fomit-frame-pointer -O2

WARN =	-Wall \
	-Wmissing-prototypes \
	-Wshadow \
	-Wcast-qual \
	-Waggregate-return

CFLAGS = -mshort $(MINTINCLUDE) $(WARN) $(OPTS)
ASFLAGS = 

FLAGS = CC="$(CC)" \
	CFLAGS="$(CFLAGS)" \
	NATIVECC="$(NATIVECC)" \
	NATIVECFLAGS="$(NATIVECFLAGS)" \
	MINTLIBINC="$(MINTLIBINC)"

#
# main target
#
all:
	cd minixfs; $(MAKE) $(FLAGS)
	cd minit; $(MAKE) $(FLAGS)
	cd fsck; $(MAKE) $(FLAGS)
	cd tools; $(MAKE) $(FLAGS)


#
# macros for cleaning up
#

clean:
	cd minixfs; $(MAKE) clean
	cd minit; $(MAKE) clean
	cd fsck; $(MAKE) clean
	cd tools; $(MAKE) clean

distclean:
	$(RM) *~ core
	cd minixfs; $(MAKE) distclean
	cd minit; $(MAKE) distclean
	cd fsck; $(MAKE) distclean
	cd tools; $(MAKE) distclean
