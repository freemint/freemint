#
# Makefile for net-tools
#
TARGETS = \
	arp \
	diald \
	ifconfig \
	iflink \
	ifstats \
	masqconf \
	netstat \
	pppconf \
	route \
	slattach

SHELL = /bin/sh
SUBDIRS = tests

srcdir = .
top_srcdir = ..
subdir = net-tools

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGETS)

# default overwrites
LIBS = -lsocket

# default definitions
OBJS = $(COBJS:.c=.o)
GENFILES = $(TARGETS)

ARPOBJS       = arp.o
DIALDOBJS     = diald.o
IFCONFIGOBJS  = ifconfig.o ifopts.o
IFLINKOBJS    = iflink.o
IFSTATSOBJS   = ifstats.o
MASQCONFOBJS  = masqconf.o
NETSTATOBJS   = netstat.o
PPPCONFOBJS   = pppconf.o
ROUTEOBJS     = route.o
SLATTACHOBJS  = slattach.o


arp: $(ARPOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

diald: $(DIALDOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

ifconfig: $(IFCONFIGOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

iflink: $(IFLINKOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

ifstats: $(IFSTATSOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

masqconf: $(MASQCONFOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

netstat: $(NETSTATOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

pppconf: $(PPPCONFOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

route: $(ROUTEOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

slattach: $(SLATTACHOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
