#
# Makefile for test tools
#
TARGETS = \
	client \
	dgram \
	dgramd \
	hostlookup \
	oobcl \
	oobsv \
	pipes \
	protolookup \
	server \
	servlookup \
	sockname \
	sockpair \
	speed \
	speed2 \
	speedd \
	tcpcl \
	tcpsv \
	udpclnt \
	udpserv

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = tests

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


CLIENTOBJS      = client.o
DGRAMOBJS       = dgram.o
DGRAMDOBJS      = dgramd.o
HOSTLOOKUPOBJS  = hostlookup.o
OOBCLOBJS       = oobcl.o
OOBSVOBJS       = oobsv.o
PIPESOBJS       = pipes.o
PROTOLOOKUPOBJS = protolookup.o
SERVEROBJS      = server.o
SERVLOOKUPOBJS  = servlookup.o
SOCKNAMEOBJS    = sockname.o
SOCKPAIROBJS    = sockpair.o
SPEEDOBJS       = speed.o
SPEED2OBJS      = speed2.o
SPEEDDOBJS      = speedd.o
TCPCLOBJS       = tcpcl.o
TCPSVOBJS       = tcpsv.o
UDPCLNTOBJS     = udpclnt.o
UDPSERVOBJS     = udpserv.o


client: $(CLIENTOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

dgram: $(DGRAMOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

dgramd: $(DGRAMDOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

hostlookup: $(HOSTLOOKUPOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

oobcl: $(OOBCLOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

oobsv: $(OOBSVOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

pipes: $(PIPESOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

protolookup: $(PROTOLOOKUPOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

server: $(SERVEROBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

servlookup: $(SERVLOOKUPOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

sockname: $(SOCKNAMEOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

sockpair: $(SOCKPAIROBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

speed: $(SPEEDOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

speed2: $(SPEED2OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

speedd: $(SPEEDDOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

tcpcl: $(TCPCLOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

tcpsv: $(TCPSVOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

udpclnt: $(UDPCLNTOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@

udpserv: $(UDPSERVOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)
	$(STRIP) $@


include $(top_srcdir)/DEPENDENCIES
