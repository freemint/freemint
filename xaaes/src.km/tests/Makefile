#
# Makefile for XaAES
#

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ..
subdir = tests

# we are doing compile tests here only in default builds,
# no need for multi-cpu targets
CPU = 000

default: tests

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

LIBS = -lgem

TEST_PROGRAMS = \
	apgetinf.tos \
	applfind.tos \
	applsrch.tos \
	colricon.prg \
	evnt_msg.prg \
	evnt_mul.tos \
	graf_hnd.tos \
	msg_ovl.gtp \
	rsc_load.gtp \
	scrp_rea.tos \
	shel_fnd.tos \
	shel_sht.prg \
	shel_wrm.prg \
	shel_wrx.prg \
	tfail.prg \
	wind_get.tos \
	windcre2.prg \
	windcre3.prg \
	windcre4.prg \
	windcrea.prg \
	wu_term.prg \
	$(empty)

all-here: tests

tests: $(TEST_PROGRAMS)
	@:

apgetinf.tos: apgetinf.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

applfind.tos: applfind.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

applsrch.tos: applsrch.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

colricon.prg: colricon.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

evnt_msg.prg: evnt_msg.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

evnt_mul.tos: evnt_mul.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

graf_hnd.tos: graf_hnd.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

inf_aes.prg: inf_aes.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

msg_ovl.gtp: msg_ovl.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

rsc_load.gtp: rsc_load.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

scrp_rea.tos: scrp_rea.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

shel_fnd.tos: shel_fnd.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

shel_sht.prg: shel_sht.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

shel_wrm.prg: shel_wrm.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

shel_wrx.prg: shel_wrx.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

tfail.prg: tfail.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

wind_get.tos: wind_get.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

windcre2.prg: windcre2.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

windcre3.prg: windcre3.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

windcre4.prg: windcre4.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

windcrea.prg: windcrea.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

wu_term.prg: wu_term.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

$(TEST_PROGRAMS): $(HEADERS)

# default overwrites

# we must remove -mshort from CFLAGS here because mintlib is not available for that configuration
CFLAGS := $(subst -mshort,,$(CFLAGS))

# default definitions

000 deb sto:
	$(MAKE) CPU=000

020:
	$(MAKE) CPU=020

030:
	$(MAKE) CPU=030

040:
	$(MAKE) CPU=040

ara:: 040
	@:

hat:: 000
	@:

hat030:: 030
	@:

060:
	$(MAKE) CPU=060

02060:
	$(MAKE) CPU=02060

col:
	$(MAKE) CPU=v4e

clean distclean::
	$(RM) *.tos *.prg *.gtp *.app
