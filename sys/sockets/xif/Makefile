#
# Makefile for 
#
TARGET = 
DRIVERS = rtl8012 rtl8012st dial slip plip dummy de600 lance pamsdma biodma \
	rieblspc rieblspc_fast rieblmst rieblmst_fast rieblste riebltt

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = xif

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: entry

# default overwrites
INCLUDES = -I.. -I$(top_srcdir)
DEFINITIONS = 

LD = $(CC) -mshort -nostdlib -Wl,--entry -Wl,_init
LIBS = $(LIBKERN) -lgcc
CPU = 000

# default definitions
OBJS = $(addsuffix .o, $(basename $(notdir $(COBJS))))

COMMON_OBJS = $(addsuffix .o, $(basename $(notdir $(COMMON_SRCS))))
DIAL_OBJS = $(addsuffix .o, $(basename $(notdir $(DIAL_SRCS))))
SLIP_OBJS = $(addsuffix .o, $(basename $(notdir $(SLIP_SRCS))))
PLIP_OBJS = $(addsuffix .o, $(basename $(notdir $(PLIP_SRCS))))
DUMMY_OBJS = $(addsuffix .o, $(basename $(notdir $(DUMMY_SRCS))))
DE600_OBJS = $(addsuffix .o, $(basename $(notdir $(DE600_SRCS))))
LANCE_OBJS = $(addsuffix .o, $(basename $(notdir $(LANCE_SRCS))))
RIEBL_SPEC_OBJS = $(addsuffix .o, $(basename $(notdir $(RIEBL_SPEC_SRCS))))
RIEBL_SPEC_FAST_OBJS = $(addsuffix .o, $(basename $(notdir $(RIEBL_SPEC_FAST_SRCS))))
RIEBL_MST_OBJS = $(addsuffix .o, $(basename $(notdir $(RIEBL_MST_SRCS))))
RIEBL_MST_FAST_OBJS = $(addsuffix .o, $(basename $(notdir $(RIEBL_MST_FAST_SRCS))))
RIEBL_MSTE_OBJS = $(addsuffix .o, $(basename $(notdir $(RIEBL_MSTE_SRCS))))
RIEBL_TT_OBJS = $(addsuffix .o, $(basename $(notdir $(RIEBL_TT_SRCS))))
PAMSDMA_OBJS = $(addsuffix .o, $(basename $(notdir $(PAMSDMA_SRCS))))
BIODMA_OBJS = $(addsuffix .o, $(basename $(notdir $(BIODMA_SRCS))))
RTL8012_OBJS = $(addsuffix .o, $(basename $(notdir $(RTL8012_SRCS))))

# entry point
entry: $(LIBKERNTARGET) $(DRIVERS)


# generic driver target
$(TARGET): $(XOBJS) $(LIBKERNSTMP)
	$(LD) $(CFLAGS) -o $@ $(XOBJS) $(LIBS)
	$(STRIP) $@


dial:
	$(MAKE) dial.xif \
		"TARGET=dial.xif" \
		"XOBJS=$(COMMON_OBJS) $(DIAL_OBJS)"

slip:
	$(MAKE) slip.xif \
		"TARGET=slip.xif" \
		"XOBJS=$(COMMON_OBJS) $(SLIP_OBJS)"

plip:
	$(MAKE) plip.xif \
		"TARGET=plip.xif" \
		"XOBJS=$(COMMON_OBJS) $(PLIP_OBJS)"

de600:
	$(MAKE) de600.xif \
		"TARGET=de600.xif" \
		"XOBJS=$(COMMON_OBJS) $(DE600_OBJS)"

dummy:
	$(MAKE) dummy.xif \
		"TARGET=dummy.xif" \
		"XOBJS=$(COMMON_OBJS) $(DUMMY_OBJS)"

lance:
	$(MAKE) lance.xif \
		"TARGET=lance.xif" \
		"XOBJS=$(COMMON_OBJS) $(LANCE_OBJS)" \
		"DEFINITIONS=-DPAMs_INTERN"

rieblspc:
	$(MAKE) rieblspc.xif \
		"TARGET=rieblspc.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_SPEC_OBJS)" \
		"DEFINITIONS=-DSPECIAL -DHACK"

rieblspc_fast:
	$(MAKE) rieblspc_fast.xif \
		"TARGET=rieblspc_fast.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_SPEC_FAST_OBJS)" \
		"DEFINITIONS=-DSPECIAL"

rieblmst:
	$(MAKE) rieblmst.xif \
		"TARGET=rieblmst.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_MST_OBJS)" \
		"DEFINITIONS=-DMEGA_ST -DHACK" \
		"PRGFLAGS=--msuper-memory"

rieblmst_fast:
	$(MAKE) rieblmst_fast.xif \
		"TARGET=rieblmst_fast.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_MST_FAST_OBJS)" \
		"DEFINITIONS=-DMEGA_ST"

rieblste:
	$(MAKE) rieblste.xif \
		"TARGET=rieblste.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_MSTE_OBJS)" \
		"DEFINITIONS=-DMEGA_STE"

riebltt:
	$(MAKE) riebltt.xif \
		"TARGET=riebltt.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_TT_OBJS)" \
		"DEFINITIONS=-DATARI_TT"

pamsdma:
	$(MAKE) pamsdma.xif \
		"TARGET=pamsdma.xif" \
		"XOBJS=$(COMMON_OBJS) $(PAMSDMA_OBJS)"

#biodma: remove the XCPPFLAGS line if you want to speed up things a little bit.
#        Please notice that standard mode is more reliable especially on faster
#        machines!
biodma:
	$(MAKE) biodma.xif \
		"TARGET=biodma.xif" \
		"XOBJS=$(COMMON_OBJS) $(BIODMA_OBJS)" \
		"DEFINITIONS=-DSTANDARD_SEND_FIRST"

rtl8012:
	$(RM) $(RTL8012_OBJS)
	$(MAKE) rtl8012.xif \
		"TARGET=rtl8012.xif" \
		"XOBJS=$(COMMON_OBJS) $(RTL8012_OBJS)"
		CPU=020-60

rtl8012st:
	$(RM) $(RTL8012_OBJS)
	$(MAKE) rtl8012st.xif \
		"TARGET=rtl8012st.xif" \
		"XOBJS=$(COMMON_OBJS) $(RTL8012_OBJS)"


include $(top_srcdir)/DEPENDENCIES
