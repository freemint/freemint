#
# the m68k arch specific files
#

SHELL = /bin/sh
SUBDIRS = asmtrans magic

srcdir = .
top_srcdir = ..
subdir = arch

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: startup objs

# default overwrites
INCLUDES = -I$(top_srcdir)
DEFINITIONS = $(KERNELDEFAULTDEFS) $(KERNELDEFS)

NOCFLAGS-mprot030.c = -m68000 -m68020 -m68040 -m68060 -m68020-60
NOCFLAGS-mprot040.c = -m68000 -m68020 -m68030 -m68040 -m68020-60
CFLAGS-mprot030.c = -m68030
CFLAGS-mprot040.c = -m68060

cflags = $(kernel_cflags)
nocflags = $(kernel_nocflags)

# default definitions
OBJS = $(COBJS:%.c=%.o) $(SOBJS:%.spp=%.o)
ASMTRANS = asmtrans/asmtrans
MAGICS = magic/magic.i
GENFILES = startup objs

startup: $(STARTUP)
	echo $(STARTUP) > $@

objs: $(OBJS)
	echo $(OBJS) > $@

#
# assembler source files
#
%.o: %.spp
	$(ASMTRANS) $(DEFINITIONS) -gas -o $*.s $<
	$(CRLF) $*.s
	$(AS) $(ASFLAGS) -c $*.s -o $@
	$(RM) $*.s	

context.o:	$(ASMTRANS) $(MAGICS) context.spp
cpu.o:		$(ASMTRANS)           cpu.spp
detect.o:	$(ASMTRANS)           detect.spp
intr.o:		$(ASMTRANS) $(MAGICS) intr.spp
kernel.o:	$(ASMTRANS) $(MAGICS) kernel.spp
memset.o:	$(ASMTRANS)           memset.spp
mmu030.o:	$(ASMTRANS)           mmu030.spp
mmu040.o:	$(ASMTRANS)           mmu040.spp
peekpoke.o:	$(ASMTRANS)           peekpoke.spp
quickzero.o:	$(ASMTRANS)           quickzero.spp
quickmove.o:	$(ASMTRANS)           quickmove.spp
quickswap.o:	$(ASMTRANS)           quickswap.spp
slb_util.o:	$(ASMTRANS) $(MAGICS) slb_util.spp
startup.o:	$(ASMTRANS)           startup.spp
syscall.o:	$(ASMTRANS) $(MAGICS) syscall.spp
vm5.o:		$(ASMTRANS)           vm5.spp
xhdi_emu.o:	$(ASMTRANS)           xhdi_emu.spp

# default dependencies
# must be included last
include $(top_srcdir)/DEPENDENCIES
