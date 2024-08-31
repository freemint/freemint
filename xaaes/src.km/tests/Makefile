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
ifeq ($(NATIVE_COLDFIRE),1)
CPU = v4e
else
CPU = 000
endif

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
	toolbar.prg \
	tbarmenu.prg \
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

# we must remove -mshort from CFLAGS here because mintlib is not available for that configuration
define TEST_TEMPLATE
$(1): $(basename $(1)).c
	$(LD) $(subst -mshort,,$(CFLAGS_$(CPU))) -o $$@ $$< $(LIBS)
endef

$(foreach TARGET,$(TEST_PROGRAMS),$(eval $(call TEST_TEMPLATE,$(TARGET))))

$(TEST_PROGRAMS): $(HEADERS)

# default overwrites

# default definitions

clean distclean::
	$(RM) *.tos *.prg *.gtp *.app
