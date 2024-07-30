#
# Makefile for test tools
#

SHELL = /bin/sh
SUBDIRS =

srcdir = .
top_srcdir = ../..
subdir = tests

default: help

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

include $(srcdir)/../NETTOOLSDEFS

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

EXES = 	client dgram dgramd hostlookup oobcl oobsv pipes protolookup \
	server servlookup sockname sockpair speed speed2 \
	speedd tcpcl tcpsv udpclnt udpserv

ALL_TARGETS = $(foreach TARGET, $(alltargets), $(foreach EXE, $(EXES), .compile_$(TARGET)/$(EXE)))

strip:
	$(STRIP) $(ALL_TARGETS)

all-targets: $(ALL_TARGETS)

# map executable names to source lists
OBJS_client = client.c $(COMMON_SRCS)
OBJS_dgram = dgram.c $(COMMON_SRCS)
OBJS_dgramd = dgramd.c $(COMMON_SRCS)
OBJS_hostlookup = hostlookup.c $(COMMON_SRCS)
OBJS_oobcl = oobcl.c $(COMMON_SRCS)
OBJS_oobsv = oobsv.c $(COMMON_SRCS)
OBJS_pipes = pipes.c $(COMMON_SRCS)
OBJS_protolookup = protolookup.c $(COMMON_SRCS)
OBJS_server = server.c $(COMMON_SRCS)
OBJS_servlookup = servlookup.c $(COMMON_SRCS)
OBJS_sockname = sockname.c $(COMMON_SRCS)
OBJS_sockpair = sockpair.c $(COMMON_SRCS)
OBJS_speed = speed.c $(COMMON_SRCS)
OBJS_speed2 = speed2.c $(COMMON_SRCS)
OBJS_speedd = speedd.c $(COMMON_SRCS)
OBJS_tcpcl = tcpcl.c $(COMMON_SRCS)
OBJS_tcpsv = tcpsv.c $(COMMON_SRCS)
OBJS_udpclnt = udpclnt.c $(COMMON_SRCS)
OBJS_udpserv = udpserv.c $(COMMON_SRCS)

#
# multi target stuff
#

define TARGET_TEMPLATE

$(1):: .compile_$(1)/$(2)

LIBS_$(1) =
OBJS_$(2)_$(1) = $(foreach OBJ, $(notdir $(basename $(OBJS_$(2)))), .compile_$(1)/$(OBJ).o)
DEFINITIONS_$(1) = $(DEFINITIONS)

.compile_$(1)/$(2): $$(OBJS_$(2)_$(1))
	$(LD) $$(LDEXTRA_$(1)) -o $$@ $$(CFLAGS_$$(CPU_$(1))) $$(OBJS_$(2)_$(1)) $$(LIBS_$(1))

endef

$(foreach TARGET,$(alltargets), $(foreach EXE, $(EXES), $(eval $(call TARGET_TEMPLATE,$(TARGET),$(EXE)))))

$(foreach TARGET,$(alltargets),$(foreach OBJ,$(COBJS),$(eval $(call CC_TEMPLATE,$(TARGET),$(OBJ)))))

ifneq (clean,$(findstring clean,$(MAKECMDGOALS)))
DEPS_MAGIC := $(shell mkdir -p $(addsuffix /.deps,$(addprefix .compile_,$(alltargets))) > /dev/null 2>&1 || :)
endif
