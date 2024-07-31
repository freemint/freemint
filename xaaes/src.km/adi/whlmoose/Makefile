#
# Makefile for moose
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = whlmoose

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/MOOSEDEFS

all-here: all-targets

# default overwrites
INCLUDES = -I$(top_srcdir)/../../sys
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=whlmoose
LIBS = -lgcc
LDEXTRA = -nostdlib -Wl,--entry,init -Wl,--fatal-warnings

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(moosetargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

EXES = moose.adi moose_w.adi

ALL_TARGETS = $(foreach TARGET, $(moosetargets), $(foreach EXE, $(EXES), .compile_$(TARGET)/$(EXE)))

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

define TARGET_TEMPLATE

$(1):: .compile_$(1)/$(2)

LIBKERN_NAME_$(1) = kern$$(CPU_$(1))
LIBKERN_DEP_$(1) = $(top_srcdir)/../../sys/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/../../sys/libkern -l$$(LIBKERN_NAME_$(1))
LIBS_$(1) = $$(LIBKERN_$(1))
OBJS_$(2)_$(1) = $(foreach OBJ, $(notdir $(basename $(OBJS_$(2)))), .compile_$(1)/$(OBJ).o)
DEFINITIONS_$(1) = $(DEFINITIONS) $$(MOOSEDEFS_$(1))

.compile_$(1)/$(2): $$(OBJS_$(2)_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(2)_$(1)) $(LIBS) $$(LIBS_$(1))

endef

define LIBKERN_TEMPLATE
$(top_srcdir)/../../sys/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/../../sys/libkern libkern$(1).a
endef

$(foreach TARGET,$(moosetargets), $(foreach EXE, $(EXES), $(eval $(call TARGET_TEMPLATE,$(TARGET),$(EXE)))))

$(foreach TARGET,$(moosetargets),$(foreach OBJ,$(sort $(COBJS)),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(filter-out deb,$(moosetargets)),$(eval $(call LIBKERN_TEMPLATE,$(CPU_$(TARGET)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(moosetargets))) > /dev/null 2>&1 || :)
endif
