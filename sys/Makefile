#
# Makefile for kernel
#

SHELL = /bin/sh
SUBDIRS = arch buildinfo gen-syscall libkern sockets usb xdd xfs
KERNELSUBDIRS = arch buildinfo

srcdir = .
top_srcdir = .
subdir = 

default: help

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-kernels

# default overwrites
KERNELSUBDIRS += $(MODULEDIRS)
DEFINITIONS = $(KERNELDEFAULTDEFS) $(KERNELDEFS)
WARN =  -Wall \
	-Wmissing-prototypes \
	-Wshadow \
	-Wpointer-arith \
	-Wcast-qual
LDEXTRA = -nostdlib
# XXX should be come from arch/
LDEXTRA += -Wl,--entry,_main -Wl,--fatal-warnings

ifeq ($(KMEMDEBUG),yes)
LDEXTRA += \
	-Wl,--wrap -Wl,_kcore \
	-Wl,--wrap -Wl,_kmalloc \
	-Wl,--wrap -Wl,_kfree \
	-Wl,--wrap -Wl,_umalloc \
	-Wl,--wrap -Wl,_ufree
endif

# default definitions
compile_all_dirs = .compile_* 
GENFILES = $(compile_all_dirs) mint/*~ sys/*~

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - sockets / xdd / xfs / usb'
	@echo '# - all-kernels'
	@echo '# - $(kerneltargets)'
	@echo '#'
	@echo '# - bakclean'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'
	@echo '# example for a Milan kernel: -> make mil'
	@echo '#'

ALL_KERNELS = $(foreach TARGET,$(kerneltargets),.compile_$(TARGET)/$(MINT_$(TARGET)))

strip:
	$(STRIP) $(ALL_KERNELS)

all-kernels: $(ALL_KERNELS)

#
# main target
#
define KERNEL_TARGET

LIBKERN_NAME_$(1) = kern$$(CPU_$(1))
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS) $$(KERNELDEFS_$(1))
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) = $(addsuffix .o, $(addprefix .compile_$(1)/,$(notdir $(basename $(COBJS)))))
MODULELIBS_$(1) = $(foreach DIR, $(MODULEDIRS_$(1)), $(DIR)/.compile_$(1)/$(notdir $(DIR)).a)
SUBDIR_OBJS_$(1) = arch/.compile_$(1)/libarch.a $$(MODULELIBS_$(1))

$(1): .compile_$(1)/$$(MINT_$(1))

arch_$(1) = arch/.compile_$(1)/startup.o arch/.compile_$(1)/libarch.a
kernelsubdirs_$(1) = $$(arch_$(1))

.compile_$(1)/$$(MINT_$(1)): buildinfo/build.h $$(kernelsubdirs_$(1)) $$(OBJS_$(1)) arch/.compile_$(1)/startup.o $$(SUBDIR_OBJS_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) $$(CFLAGS_$$(CPU_$(1))) -o $$@ \
		-Wl,-Map=.compile_$(1)/map.txt \
		arch/.compile_$(1)/startup.o \
		$$(OBJS_$(1)) $$(SUBDIR_OBJS_$(1)) \
		$$(LIBS_$(1))

.compile_$(1)/cdef.h:
	(cd $(srcdir)/buildinfo; \
	CC="$(CC)" \
	OPTS="$(filter-out $(nocflags), $$(MODEL_$$(CPU_$(1))) $(GENERAL) $(OPTS))" \
	DEFS="$(filter-out $(nocflags), $$(DEFINITIONS_$(1)))" \
	sh cdef.sh) > $$@

CFLAGS-$(1)-info.c = -I$(srcdir)/.compile_$(1)

.compile_$(1)/info.o: .compile_$(1)/cdef.h buildinfo/build.h

$$(arch_$(1)):
	$(MAKE) -C arch $(1)

endef

xfs/hostfs/.compile_ara/hostfs.a:
	$(MAKE) -C xfs/hostfs ara

buildinfo/build.h: $(srcdir)/buildinfo/mkbuild
	cd $(srcdir)/buildinfo; ./mkbuild

$(srcdir)/buildinfo/mkbuild:
	$(MAKE) -C $(srcdir)/buildinfo

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern libkern$(1).a
endef

$(foreach TARGET,$(kerneltargets),$(eval $(call KERNEL_TARGET,$(TARGET))))

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(kerneltargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))


sockets xdd xfs usb:
	$(MAKE) -C $@ all


ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(kerneltargets))) > /dev/null 2>&1 || :)
endif
