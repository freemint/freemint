#
# Makefile for the strace system tool
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = strace

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/STRACEDEFS

all-here: all-targets

# default overwrites
NATIVECFLAGS += -I$(top_srcdir)/../shared
NATIVE_LIBS_DEP += $(top_srcdir)/../shared/libsyscall/libsyscall.a
NATIVE_LIBS += -L$(top_srcdir)/../shared/libsyscall -lsyscall
#NATIVE_LIBS += -lfl

MASTER = $(top_srcdir)/../sys/syscalls.master
DEFINITIONS += -D_GNU_SOURCE

NOCFLAGS-print.c = -Wcast-qual

INCLUDES += -I$(srcdir) -I.

# default definitions
compile_all_dirs = .compile_*
GENFILES = $(compile_all_dirs) sysenttab.* gentables genstamp

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

EXES = 	strace

ALL_TARGETS = $(foreach TARGET, $(alltargets), $(foreach EXE, $(EXES), .compile_$(TARGET)/$(EXE)))

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

OBJS_strace = print.c strace.c sysenttab.c

sysenttab.c sysenttab.h: genstamp

genstamp: $(MASTER) gentables
	./gentables $<
	touch $@

gentables.o: gentables.c $(top_srcdir)/../shared/libsyscall/syscalldefs.h
	$(NATIVECC) $(NATIVECFLAGS) -c $< -o $@

gentables: gentables.o $(NATIVE_LIBS_DEP)
	$(NATIVECC) -o $@ $(NATIVECFLAGS) $< $(NATIVE_LIBS)

$(top_srcdir)/../shared/libsyscall/libsyscall.a:
	$(MAKE) -C $(top_srcdir)/../shared/libsyscall

#
# multi target stuff
#

LIBS =

define TARGET_TEMPLATE

$(1):: .compile_$(1)/$(2)

LIBS_$(1) =
OBJS_$(2)_$(1) = $(foreach OBJ, $(notdir $(basename $(OBJS_$(2)))), .compile_$(1)/$(OBJ).o)
DEFINITIONS_$(1) = $(DEFINITIONS)

.compile_$(1)/sysenttab.o .compile_$(1)/strace.o: sysenttab.h

.compile_$(1)/$(2): $$(OBJS_$(2)_$(1))
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(2)_$(1)) $(LIBS) $$(LIBS_$(1))

endef

$(foreach TARGET,$(alltargets), $(foreach EXE, $(EXES), $(eval $(call TARGET_TEMPLATE,$(TARGET),$(EXE)))))

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS) sysenttab.c,$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(alltargets))) > /dev/null 2>&1 || :)
endif
