#
# Makefile for USB tool
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = usbtool

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/USBTOOLDEFS

all-here: all-targets

# default overwrites
INCLUDES = -I$(top_srcdir)/../sys
ifeq (yes,$(LIBCMINI))
INCLUDES += -I$(LIBCMINI_INCPATH)
endif
OPTS += -fno-builtin

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs)

help:
	@echo '#'
	@echo '# targets:'
	@echo '# --------'
	@echo '# - all'
	@echo '# - $(alltargets)'
	@echo '#'
	@echo '# - clean'
	@echo '# - distclean'
	@echo '# - bakclean'
	@echo '# - strip'
	@echo '# - help'
	@echo '#'

EXES = 	usbtool.acc

ALL_TARGETS = $(foreach TARGET, $(alltargets), $(foreach EXE, $(EXES), .compile_$(TARGET)/$(EXE)))

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

LIBS = -lgem

define TARGET_TEMPLATE

$(1):: .compile_$(1)/$(2)

LIBS_$(1) =
OBJS_$(2)_$(1) = $(foreach OBJ, $(notdir $(basename $(OBJS_$(2)))), .compile_$(1)/$(OBJ).o)
DEFINITIONS_$(1) = $(DEFINITIONS)

ifneq (yes,$(LIBCMINI))
.compile_$(1)/$(2): $$(OBJS_$(2)_$(1))
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(2)_$(1)) $(LIBS) $$(LIBS_$(1))
else
.compile_$(1)/$(2): $$(OBJS_$(2)_$(1)) $$(LIBCMINI_DEP_$(1))
	$(LD) -nostdlib -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(LIBCMINI_STARTUP_$(1)) $$(OBJS_$(2)_$(1)) -L$$(LIBCMINI_LIBPATH_$(1)) -lcmini -lgcc -lcmini $(LIBS) $$(LIBS_$(1)) -lgcc
endif

endef

ifeq (yes,$(LIBCMINI))
$(foreach TARGET,$(alltargets),$(eval $(call LIBCMINI_TEMPLATE,$(TARGET))))
endif

$(foreach TARGET,$(alltargets), $(foreach EXE, $(EXES), $(eval $(call TARGET_TEMPLATE,$(TARGET),$(EXE)))))

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(alltargets))) > /dev/null 2>&1 || :)
endif
