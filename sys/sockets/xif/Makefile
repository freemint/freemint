#
# Makefile for most inet drivers
#
DRIVERS = rtl8012 rtl8012st dial plip dummy de600 lance pamsdma biodma \
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
GENFILES = *.xif
SGENFILES = rieblspc.c rieblspc_fast.c rieblmst.c rieblmst_fast.c rieblste.c riebltt.c rtl8012st.c


OBJS = $(addsuffix .o, $(basename $(notdir $(COBJS))))

COMMON_OBJS = $(basename $(notdir $(COMMON_SRCS)))
OBJS_dial = $(basename $(notdir $(DIAL_SRCS)))
OBJS_slip = $(basename $(notdir $(SLIP_SRCS)))
OBJS_plip = $(basename $(notdir $(PLIP_SRCS)))
OBJS_dummy = $(basename $(notdir $(DUMMY_SRCS)))
OBJS_de600 = $(basename $(notdir $(DE600_SRCS)))
OBJS_lance = $(basename $(notdir $(LANCE_SRCS)))
OBJS_rieblspc = $(basename $(notdir $(RIEBL_SPEC_SRCS)))
OBJS_rieblspc_fast = $(basename $(notdir $(RIEBL_SPEC_FAST_SRCS)))
OBJS_rieblmst = $(basename $(notdir $(RIEBL_MST_SRCS)))
OBJS_rieblmst_fast = $(basename $(notdir $(RIEBL_MST_FAST_SRCS)))
OBJS_rieblste = $(basename $(notdir $(RIEBL_MSTE_SRCS)))
OBJS_riebltt = $(basename $(notdir $(RIEBL_TT_SRCS)))
OBJS_pamsdma = $(basename $(notdir $(PAMSDMA_SRCS)))
OBJS_biodma = $(basename $(notdir $(BIODMA_SRCS)))
OBJS_rtl8012 = $(basename $(notdir $(RTL8012_SRCS)))
OBJS_rtl8012st = $(basename $(notdir $(RTL8012ST_SRCS)))
OBJS_rtl8139 = $(basename $(notdir $(RTL8139_SRCS)))
OBJS_asix = $(basename $(notdir $(ASIX_SRCS)))
OBJS_picowifi = $(basename $(notdir $(PICOWIFI_SRCS)))

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

LD = $(CC) -nostdlib -Wl,--entry,init -Wl,--fatal-warnings

define LIBKERN_TEMPLATE

COMMON_OBJS_$(1) = $(addsuffix _$(1).o,$(COMMON_OBJS))

$(top_srcdir)/libkern/libkern$(1).a:
	$(MAKE) -C $(top_srcdir)/libkern CPU="$(1)" libkern$(1).a
endef

define TARGET_TEMPLATE

LIBKERN_NAME_$(1) = kern$(1)
LIBKERN_DEP_$(1) = $(top_srcdir)/libkern/lib$$(LIBKERN_NAME_$(1)).a
LIBKERN_$(1) = -L$(top_srcdir)/libkern -l$$(LIBKERN_NAME_$(1))
DEFINITIONS_$(1) = $(DEFINITIONS) $$(XDDDEFS_$(1))
LIBS_$(1) = $$(LIBKERN_$(1)) -lgcc
OBJS_$(2)_$(1) = $(addsuffix _$(1).o, $(OBJS_$(2)))

$(2):: $(2)_$(1).xif

$(2)_$(1).xif: $$(COMMON_OBJS_$(1)) $$(OBJS_$(2)_$(1)) $$(LIBKERN_DEP_$(1))
	$(LD) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(COMMON_OBJS_$(1)) $$(OBJS_$(2)_$(1)) $$(LIBS_$(1))

endef

define CC_TEMPLATE
$(2)_$(1).o: $(2).c
	$(CC) $(INCLUDES) $$(DEFINITIONS_$(1)) $$(MODEL_$$(CPU_$(1))) $$(XIF_DEFS_$(2)) $(GENERAL) $(OPTS) $(WARN) $(CPPFLAGS) -Wp,-MD,.deps/$(2)_$(1).P -c $$< -o $$@

-include .deps/$(2)_$(1).P

endef

define AS_TEMPLATE
$(2)_$(1).o: $(2).S
	$(CC) $(INCLUDES) $$(DEFINITIONS_$(1)) $$(MODEL_$$(CPU_$(1))) $$(XIF_DEFS_$(2)) $$(XIF_LDFLAGS_$(2)) $(GENERAL) $(OPTS) $(WARN) $(CPPFLAGS) -Wp,-MD,.deps/$(2)_$(1).P -c $$< -o $$@

-include .deps/$(2)_$(1).P

endef

$(foreach TARGET,$(cputargets),$(eval $(call LIBKERN_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(cputargets),$(foreach OBJ,$(sort $(basename $(filter %.c,$(notdir $(COBJS))))),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(foreach OBJ,$(sort $(basename $(filter %.S,$(notdir $(COBJS))))),$(eval $(call AS_TEMPLATE,$(TARGET),$(OBJ)))))

$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),asix)))
$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),dial)))
$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),slip)))
$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),plip)))
$(foreach TARGET,$(filter-out v4e,$(cputargets)),$(eval $(call TARGET_TEMPLATE,$(TARGET),picowifi)))
$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),dummy)))
$(foreach TARGET,000,$(eval $(call TARGET_TEMPLATE,$(TARGET),de600)))
$(foreach TARGET,$(cputargets),$(eval $(call TARGET_TEMPLATE,$(TARGET),lance)))
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

echo:
	@echo $(LIBKERN_NAME_v4e)
