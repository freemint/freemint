#
# Makefile for XaAES
#

SHELL = /bin/sh
SUBDIRS = adi xaloader gradient tests

srcdir = .
top_srcdir = .
subdir = src.km

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: all-targets

# default overwrites
DEFINITIONS = -D__KERNEL_MODULE__ -DMODULE_NAME=XaAES

INCLUDES += -I$(top_srcdir)/../../sys
WARN += -Wno-aggregate-return -Winline -Wdeclaration-after-statement -Werror

LDEXTRA = -nostdlib -Wl,--entry,init_km -Wl,--fatal-warnings
LIBS = -lgem -lgcc
STRIPFLAGS =

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) *.km
SGENFILES = xversion.h

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(xaaestargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

ALL_TARGETS = $(foreach TARGET,$(xaaestargets),.compile_$(TARGET)/$(XAAESTARGET_$(TARGET)))

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

xversion.h: $(SRCFILES)
	$(SHELL) xversion.sh $@ $?

check test::
	test ! -f tests/Makefile || $(MAKE) -C tests $@

#
# multi target stuff
#
LIBS = -lgem -lgcc

define XAAES_TARGET

DEFINITIONS_$(1) = $(DEFINITIONS) $$(XAAESDEFS_$(1))
LIBS_$(1) =
OBJS_$(1) = $(foreach OBJ, $(notdir $(basename $(COBJS))), .compile_$(1)/$(OBJ).o)

$(1): .compile_$(1)/$$(XAAESTARGET_$(1))

$$(OBJS_$(1)): XAAESDEFS

.compile_$(1)/$$(XAAESTARGET_$(1)): $$(OBJS_$(1))
	$(LD) $$(CFLAGS_$(CPU_$(1))) -o $$@ $$(OBJS_$(1)) $(LIBS) $$(LIBS_$(1))

.compile_$(1)/info.o .compile_$(1)/about.o: xversion.h

endef

$(foreach TARGET,$(xaaestargets),$(eval $(call XAAES_TARGET,$(TARGET))))

$(foreach TARGET,$(xaaestargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))


ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(xaaestargets))) > /dev/null 2>&1 || :)
endif
