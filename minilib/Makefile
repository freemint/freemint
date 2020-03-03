top_srcdir = ..

default: all
	@:

ifeq ($(srcdir),)
srcdir=.
endif
include $(srcdir)/$(top_srcdir)/CONFIGVARS

OPTS := -Os -fomit-frame-pointer

CPPFLAGS += -I$(srcdir) -I$(srcdir)/include

ifeq ($(target),)
CFLAGS += -m68000
endif

ifeq ($(target),mshort)
CFLAGS += -m68000 -mshort
endif

ifeq ($(target),m68020-60)
CFLAGS += -m68020-60
endif

ifeq ($(target),m68020-60/mshort)
CFLAGS += -m68020-60 -mshort
endif

ifeq ($(target),m5475)
CFLAGS += -mcpu=5475
endif

ifeq ($(target),m5475/mshort)
CFLAGS += -mcpu=5475 -mshort
endif

DIRS = . m68020-60 m5475 mshort m68020-60/mshort m5475/mshort

include $(srcdir)/SRCFILES

all:: .dirs $(addsuffix /libmini.a,$(DIRS)) crt0.o minicrt0.o

./libmini.a: $(OBJS)
	$(RM) $@
	$(AR) rcs $@ $(OBJS)

mshort/libmini.a: .dirs .force
	$(MAKE) -C mshort -f ../Makefile srcdir=.. target=mshort ./libmini.a

m68020-60/libmini.a: .dirs .force
	$(MAKE) -C m68020-60 -f ../Makefile srcdir=.. target=m68020-60 ./libmini.a

m68020-60/mshort/libmini.a: .dirs .force
	$(MAKE) -C m68020-60/mshort -f ../../Makefile srcdir=../.. target=m68020-60/mshort ./libmini.a

m5475/libmini.a: .dirs .force
	$(MAKE) -C m5475 -f ../Makefile srcdir=.. target=m5475 ./libmini.a

m5475/mshort/libmini.a: .dirs .force
	$(MAKE) -C m5475/mshort -f ../../Makefile srcdir=../.. target=m5475/mshort ./libmini.a

.dirs:
	-for i in $(DIRS); do mkdir -p $$i; done
	touch $@

.force:

clean::
	$(RM) *.a *.o
	$(RM) -rf m68020-60 mshort m5475
	$(RM) .dirs
