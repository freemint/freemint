#
# the keyboard table translator program
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ..
subdir = mktbl

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/MKTBLDEFS

all-here: all-targets

# default overwrites

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

ALL_TARGETS = $(foreach TARGET,$(alltargets),.compile_$(TARGET)/mktbl)

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

#
# multi target stuff
#

define TARGET_TEMPLATE

$(1): .compile_$(1)/mktbl

LIBS_$(1) =
OBJS_$(1) = $(foreach OBJ, $(notdir $(basename $(COBJS))), .compile_$(1)/$(OBJ).o)
DEFINITIONS_$(1) = $(DEFINITIONS)

.compile_$(1)/mktbl: $$(OBJS_$(1))
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(1)) $$(LIBS_$(1))

endef

$(foreach TARGET,$(alltargets),$(eval $(call TARGET_TEMPLATE,$(TARGET))))

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

TBLDIR = ../../sys/tbl

compile-tables:
	$(NATIVECC) $(NATIVECFLAGS) -DMAIN -o mktbl mktbl.c unitable.c
	./mktbl keyboards/en_uk.src $(TBLDIR)/en_uk.tbl
	./mktbl keyboards/cs_cz-is.src $(TBLDIR)/cs_cz-is.tbl
	./mktbl keyboards/cs_cz-ka.src $(TBLDIR)/cs_cz-ka.tbl
	./mktbl keyboards/nl_nl.src $(TBLDIR)/nl_nl.tbl
	./mktbl keyboards/fi_fi.src $(TBLDIR)/fi_fi.tbl
	./mktbl keyboards/fr_fr.src $(TBLDIR)/fr_fr.tbl
	./mktbl keyboards/fr_fr-pc.src $(TBLDIR)/fr_fr-pc.tbl
	./mktbl keyboards/de_de.src $(TBLDIR)/de_de.tbl
	./mktbl keyboards/el_gr.src $(TBLDIR)/el_gr.tbl
	./mktbl keyboards/hu_hu.src $(TBLDIR)/hu_hu.tbl
	./mktbl keyboards/nn_no.src $(TBLDIR)/nn_no.tbl
	./mktbl keyboards/it_it.src $(TBLDIR)/it_it.tbl
	./mktbl keyboards/ru_ru.src $(TBLDIR)/ru_ru.tbl
	./mktbl keyboards/es_es.src $(TBLDIR)/es_es.tbl
	./mktbl keyboards/sv_se.src $(TBLDIR)/sv_se.tbl
	./mktbl keyboards/de_ch.src $(TBLDIR)/de_ch.tbl
	./mktbl keyboards/fr_ch.src $(TBLDIR)/fr_ch.tbl
	./mktbl keyboards/en_us-pc.src $(TBLDIR)/en_us-pc.tbl
	./mktbl keyboards/en_uk-pl.src $(TBLDIR)/en_uk-pl.tbl
	./mktbl keyboards/fr_fr-pl.src $(TBLDIR)/fr_fr-pl.tbl
	./mktbl keyboards/de_de-pl.src $(TBLDIR)/de_de-pl.tbl
	./mktbl keyboards/it_it-pl.src $(TBLDIR)/it_it-pl.tbl
	./mktbl keyboards/es_es-pl.src $(TBLDIR)/es_es-pl.tbl
	./mktbl keyboards/de_ch-pl.src $(TBLDIR)/de_ch-pl.tbl
	./mktbl keyboards/fr_ch-pl.src $(TBLDIR)/fr_ch-pl.tbl
	./mktbl keyboards/en_us-pl.src $(TBLDIR)/en_us-pl.tbl
	./mktbl keyboards/ro_ro.src $(TBLDIR)/ro_ro.tbl
	$(RM) mktbl

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(alltargets))) > /dev/null 2>&1 || :)
endif
