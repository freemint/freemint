#
# Makefile for 
#
TARGET = 
DRIVERS = rtl8012 rtl8012st rtl8012ct60 dial plip dummy de600 lance pamsdma biodma \
	rieblspc rieblspc_fast rieblmst rieblmst_fast rieblste riebltt asix rtl8139 picowifi

SHELL = /bin/sh
SUBDIRS = nfeth daynaport ethernat fec svethlana v2expeth v4net

srcdir = .
top_srcdir = ../..
subdir = xif

default: all

CPU = 000

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: entry

strip:
	$(STRIP) *.xif

# default overwrites
INCLUDES = -I.. -I$(top_srcdir)
XIF_DEFINITIONS = 
DEFINITIONS = -D__KERNEL_XDD__ $(XIF_DEFINITIONS)

LD = $(CC) -mshort -nostdlib -Wl,--entry,init -Wl,--fatal-warnings
LIBS = $(LIBKERN) -lgcc

# default definitions
GENFILES = *.xif
SGENFILES = rieblspc.c rieblspc_fast.c rieblmst.c rieblmst_fast.c rieblste.c riebltt.c rtl8012st.c rtl8012ct60.c main_020.c 


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
RTL8012ST_OBJS = $(addsuffix .o, $(basename $(notdir $(RTL8012ST_SRCS))))
RTL8012CT60_OBJS = $(addsuffix .o, $(basename $(notdir $(RTL8012CT60_SRCS))))
RTL8139_OBJS = $(addsuffix .o, $(basename $(notdir rtl8139.c)))
ASIX_OBJS = $(addsuffix .o, $(basename $(notdir $(ASIX_SRCS))))
PICOWIFI_OBJS = $(addsuffix .o, $(basename $(notdir $(PICOWIFI_SRCS))))

# entry point
entry: $(DRIVERS)


# generic driver target
$(TARGET): $(XOBJS) $(LIBKERNTARGET)
	$(LD) $(CFLAGS) -o $@ $(XOBJS) $(LIBS)


asix:
	$(MAKE) asix.xif \
		"TARGET=asix.xif" \
		"XOBJS=$(COMMON_OBJS) $(ASIX_OBJS)"

picowifi:
	$(MAKE) picowifi.xif \
		"TARGET=picowifi.xif" \
		"XOBJS=$(COMMON_OBJS) $(PICOWIFI_OBJS)"

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
		"XIF_DEFINITIONS=-DPAMs_INTERN"

rieblspc:
	$(MAKE) rieblspc.xif \
		"TARGET=rieblspc.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_SPEC_OBJS)" \
		"XIF_DEFINITIONS=-DSPECIAL -DHACK"

rieblspc.c:
	ln -s lance.c $@

rieblspc_fast:
	$(MAKE) rieblspc_fast.xif \
		"TARGET=rieblspc_fast.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_SPEC_FAST_OBJS)" \
		"XIF_DEFINITIONS=-DSPECIAL"

rieblspc_fast.c:
	ln -s lance.c $@

rieblmst:
	$(MAKE) rieblmst.xif \
		"TARGET=rieblmst.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_MST_OBJS)" \
		"XIF_DEFINITIONS=-DMEGA_ST -DHACK" \
		"PRGFLAGS=--msuper-memory"

rieblmst.c:
	ln -s lance.c $@

rieblmst_fast:
	$(MAKE) rieblmst_fast.xif \
		"TARGET=rieblmst_fast.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_MST_FAST_OBJS)" \
		"XIF_DEFINITIONS=-DMEGA_ST"

rieblmst_fast.c:
	ln -s lance.c $@

rieblste:
	$(MAKE) rieblste.xif \
		"TARGET=rieblste.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_MSTE_OBJS)" \
		"XIF_DEFINITIONS=-DMEGA_STE"

rieblste.c:
	ln -s lance.c $@

riebltt:
	$(MAKE) riebltt.xif \
		"TARGET=riebltt.xif" \
		"XOBJS=$(COMMON_OBJS) $(RIEBL_TT_OBJS)" \
		"XIF_DEFINITIONS=-DATARI_TT"

riebltt.c:
	ln -s lance.c $@

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
		"XIF_DEFINITIONS=-DSTANDARD_SEND_FIRST"

rtl8012:
	$(MAKE) rtl8012.xif \
		"TARGET=rtl8012.xif" \
		"XOBJS=main_020.o $(RTL8012_OBJS)" \
		CPU=020-60
main_020.o: main_020.c

rtl8012st:
	$(MAKE) rtl8012st.xif \
		"TARGET=rtl8012st.xif" \
		"XOBJS=$(COMMON_OBJS) $(RTL8012ST_OBJS)" \
		"XIF_DEFINITIONS=-DNO_DELAY"

rtl8012st.c:
	ln -s rtl8012.c $@

rtl8012ct60:
	$(MAKE) rtl8012ct60.xif \
		"TARGET=rtl8012ct60.xif" \
		"XOBJS=main_020.o $(RTL8012CT60_OBJS)" \
		CPU=060

rtl8012ct60.c:
	ln -s rtl8012.c $@

rtl8139:
	$(MAKE) rtl8139.xif \
		"TARGET=rtl8139.xif" \
		"XOBJS=main_020.o $(RTL8139_OBJS)" \
		CPU=060

main_020.c: main.c
	ln -s $< $@

include $(top_srcdir)/DEPENDENCIES
