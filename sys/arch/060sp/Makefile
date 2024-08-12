#
# the m68k 060sp
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = 060sp

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

# default overwrites
LDEXTRA = -nostdlib -Wl,--entry,_init

alltargets = 060

ALL_TARGETS = 060sp.prg

all-here: $(ALL_TARGETS)

# default definitions
compile_all_dirs = .compile_* 
GENFILES = $(compile_all_dirs) fpsp.S isp.S fpsp.sa isp.sa *.prg *.bin dfpsp

OBJS = $(foreach OBJ, $(notdir $(basename $(COBJS))), .compile_060/$(OBJ).o)

strip:
	$(STRIP) $(ALL_TARGETS)

060sp.prg: $(OBJS)
	$(LD) $(CFLAGS_060) -o $@ $(OBJS) $(LDEXTRA)

.compile_060/mint060sp.o: fpsp.sa isp.sa

fpsp.sa: fpsp.bin dfpsp
	./dfpsp $< > $@

isp.sa: isp.bin dfpsp
	./dfpsp $< > $@

ASFLAGS = $(CFLAGS_060)
LDFLAGS = -s

fpsp.bin: $(FPSP060_SRCS) Makefile
	cat $(FPSP060_SRCS) > tmp.S
	@$(CC) $(ASFLAGS) -c -o tmp.o tmp.S
	@if $(OBJDUMP) -r tmp.o | grep -w 32; then \
		$(OBJDUMP) -r tmp.o; \
		echo "$< contains relocations" >&2; \
		exit 1; \
	fi
	@$(RM) ftest.o
	$(CC) $(ASFLAGS) $(LDFLAGS) -Wl,--oformat,binary -Wl,-entry,_start -o $@ -nostartfiles -nostdlib tmp.S
	$(RM) tmp.S tmp.o

isp.bin: $(ISP060_SRCS) Makefile
	@$(CC) $(ASFLAGS) -c -o isp.o $<
	@if $(OBJDUMP) -r isp.o | grep -w 32; then \
		$(OBJDUMP) -r isp.o; \
		echo "$< contains relocations" >&2; \
		exit 1; \
	fi
	@$(RM) isp.o
	$(CC) $(ASFLAGS) $(LDFLAGS) -Wl,--oformat,binary -Wl,-entry,_start -o $@ -nostartfiles -nostdlib $<

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

skel060.o: skel060.S fpsp.sa isp.sa

dfpsp: dfpsp.c
	$(NATIVECC) -o $@ $<

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(foreach DIR, $(alltargets), .compile_$(DIR)/.deps) > /dev/null 2>&1 || :)
endif
