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

diald: $(DIALDOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

ifconfig: $(IFCONFIGOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

iflink: $(IFLINKOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

ifstats: $(IFSTATSOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

masqconf: $(MASQCONFOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

netstat: $(NETSTATOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

pppconf: $(PPPCONFOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

route: $(ROUTEOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

slattach: $(SLATTACHOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)


include $(top_srcdir)/DEPENDENCIES
