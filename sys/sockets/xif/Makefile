#
# Makefile for most inet drivers
#
DRIVERS = rtl8012 rtl8012st dial slip plip dummy de600 lance pamsdma biodma \
	rieblspc rieblspc_fast rieblmst rieblmst_fast rieblste riebltt asix rtl8139 picowifi

SHELL = /bin/sh
SUBDIRS = nfeth daynaport ethernat fec svethlana v2expeth v4net

srcdir = .
top_srcdir = ../..
subdir = xif

default: all

# Disable built-in rules
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(DRIVERS)

strip:
	$(STRIP) *.xif

# default overwrites
INCLUDES = -I.. -I$(top_srcdir)
DEFINITIONS = -D__KERNEL_XDD__

# default definitions
compile_all_dirs = .deps
GENFILES = $(compile_all_dirs) *.xif
SGENFILES = rieblspc.c rieblspc_fast.c rieblmst.c rieblmst_fast.c rieblste.c riebltt.c rtl8012st.c


OBJS = $(addsuffix .o, $(basename $(notdir $(COBJS))))

# map driver names to source lists
COMMON_OBJS = $(COMMON_SRCS)
OBJS_dial = $(DIAL_SRCS)
OBJS_slip = $(SLIP_SRCS)
OBJS_plip = $(PLIP_SRCS)
OBJS_dummy = $(DUMMY_SRCS)
OBJS_de600 = $(DE600_SRCS)
OBJS_lance = $(LANCE_SRCS)
OBJS_rieblspc = $(RIEBL_SPEC_SRCS)
OBJS_rieblspc_fast = $(RIEBL_SPEC_FAST_SRCS)
OBJS_rieblmst = $(RIEBL_MST_SRCS)
OBJS_rieblmst_fast = $(RIEBL_MST_FAST_SRCS)
OBJS_rieblste = $(RIEBL_MSTE_SRCS)
OBJS_riebltt = $(RIEBL_TT_SRCS)
OBJS_pamsdma = $(PAMSDMA_SRCS)
OBJS_biodma = $(BIODMA_SRCS)
OBJS_rtl8012 = $(RTL8012_SRCS)
OBJS_rtl8012st = $(RTL8012ST_SRCS)
OBJS_rtl8139 = $(RTL8139_SRCS)
OBJS_asix = $(ASIX_SRCS)
OBJS_picowifi = $(PICOWIFI_SRCS)

XIF_DEFS_lance = -DPAMs_INTERN
XIF_DEFS_rieblspc = -DSPECIAL -DHACK
XIF_DEFS_rieblspc_fast = -DSPECIAL
XIF_DEFS_rieblmst = -DMEGA_ST -DHACK
XIF_LDFLAGS_rieblmst = --msuper-memory
XIF_DEFS_rieblmst_fast = -DMEGA_ST
XIF_DEFS_rieblste = -DMEGA_STE
XIF_DEFS_riebltt = -DATARI_TT
#biodma: remove the XCPPFLAGS line if you want to speed up things a little bit.
#        Please notice that standard mode is more reliable especially on faster
#        machines!
XIF_DEFS_biodma = -DSTANDARD_SEND_FIRST
XIF_DEFS_rtl8012st = -DNO_DELAY

rieblspc.c:
	ln -s lance.c $@

rieblspc_fast.c:
	ln -s lance.c $@

rieblmst.c:
	ln -s lance.c $@

rieblmst_fast.c:
	ln -s lance.c $@

rieblste.c:
	ln -s lance.c $@

riebltt.c:
	ln -s lance.c $@

rtl8012st.c:
	ln -s rtl8012.c $@



#
# multi target stuff
#

LDEXTRA = -nostdlib -Wl,--entry,init -Wl,--fatal-warnings

define LIBKERN_TEMPLATE

COMMON_OBJS_$(1) = $(addsuffix _$(1).o, $(basename $(notdir $(COMMON_OBJS))))

$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern libkern$(1).a
endef

define TARGET_TEMPLATE

LIBKERN_NAME_$(1) = kern$(1)
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS)
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(2)_$(1) = $(addsuffix _$(1).o, $(basename $(notdir $(OBJS_$(2)))))

$(2):: $(2)_$(1).xif

$(2)_$(1).xif: $$(COMMON_OBJS_$(1)) $$(OBJS_$(2)_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(COMMON_OBJS_$(1)) $$(OBJS_$(2)_$(1)) $$(LIBS_$(1))

endef

define CPU_TEMPLATE
$(basename $(2))_$(1).o: $(2)
	$(CC) $(INCLUDES) $$(DEFINITIONS_$(1)) $$(XIF_DEFS_$(basename $(2))) $$(MODEL_$(1)) $(GENERAL) $(OPTS) $(WARN) $(CPPFLAGS) -Wp,-MD,.deps/$(basename $(2))_$(1).P -Wp,-MT,$$@ -c $$< -o $$@

-include .deps/$(basename $(2))_$(1).P

endef

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

# sorting is needed here to eliminate duplicates
$(foreach TARGET,$(cputargets),$(foreach OBJ,$(sort $(notdir $(COBJS))),$(eval $(call CPU_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),asix)))
$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),dial)))
$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),slip)))
$(foreach TARGET,$(filter-out v4e,$(cputargets)),$(eval $(call TARGET_TEMPLATE,$(TARGET),plip)))
$(foreach TARGET,$(filter-out v4e,$(cputargets)),$(eval $(call TARGET_TEMPLATE,$(TARGET),picowifi)))
$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),dummy)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),de600)))
$(foreach TARGET,$(filter-out v4e,$(cputargets)),$(eval $(call TARGET_TEMPLATE,$(TARGET),lance)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),rieblspc)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),rieblspc_fast)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),rieblmst)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),rieblmst_fast)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),rieblste)))
$(foreach TARGET,02060 030 040 060,$(eval $(call TARGET_TEMPLATE,$(TARGET),riebltt)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),pamsdma)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),biodma)))
$(foreach TARGET,02060 030 040 060,$(eval $(call TARGET_TEMPLATE,$(TARGET),rtl8012)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),rtl8012st)))
$(foreach TARGET,060,$(eval $(call TARGET_TEMPLATE,$(TARGET),rtl8139)))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p .deps) > /dev/null 2>&1 || :)
endif
