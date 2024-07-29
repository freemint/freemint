#
# Makefile for hostfs.xfs
#
# If you got these sources as part of STonX note:
#    - You have to copy all files of this directory into the FreeMiNT source
#      tree at "sys/xfs/hostfs". Instead of you may change top_srcdir at this
#      Makefile to "sys" directory of the MiNT source tree.
#      "make" does only work, if you do so!
#
# Note: You need a crosscompiler to make hostfs.xfs or a native m68k-atari-mint
#       gcc, make etc.
#       To use crosscompiling change the variables at CONFIGVARS.
#       To crosscompile even FreeMiNT, you may have to change CRLF at
#       CONFIGVARS (e.g. say "CRLF = echo crlf"). There may be additional
#       errors, because of using "flags" at some sub-Makefiles. Comment out
#       "flags"-calls at those Makefiles (resulting programms will only work
#       if you're running at least version 1.15.10 of FreeMiNT.
#
SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = hostfs

default: all

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

# default overwrites
# for aranym, this is compiled into the kernel
ifneq ($(kernel),)
DEFINITIONS = $(KERNELDEFAULTDEFS) $(KERNELDEFS)
targets = ara
TARGET = .compile_ara/objs
else
DEFINITIONS = -D__KERNEL_XFS__
targets = 000
COBJS += $(CMODOBJS)
TARGET = hostfs.xfs
endif
INCLUDES = -I$(top_srcdir)

# default definitions
all_compile_dirs = .compile_*
GENFILES = $(all_compile_dirs) $(TARGET)

all-here: $(TARGET)

strip:
	$(STRIP) $(TARGET)

LD = $(CC) -nostdlib -Wl,--entry,init_xfs -Wl,--fatal-warnings -s

define TARGET_TEMPLATE

LIBKERN_NAME_$(1) = kern$$(CPU_$(1))
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS)
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(1) = $(addsuffix .o, $(addprefix .compile_$(1)/,$(notdir $(basename $(COBJS)))))

# util.sh currently expects it in this folder
hostfs.xfs: $$(OBJS_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))

.compile_$(1)/objs: $$(OBJS_$(1))
	@echo "Kernel: \"$(kernel)\""
	echo $(addsuffix .o, $(notdir $(basename $(COBJS)))) > $$@

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern CPU="$(1)" libkern$(1).a
endef

$(foreach TARGET,$(targets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(targets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(targets))) > /dev/null 2>&1 || :)
endif
