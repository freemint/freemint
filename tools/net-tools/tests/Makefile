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

dgram: $(DGRAMOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

dgramd: $(DGRAMDOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

hostlookup: $(HOSTLOOKUPOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

oobcl: $(OOBCLOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

oobsv: $(OOBSVOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

pipes: $(PIPESOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

protolookup: $(PROTOLOOKUPOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

server: $(SERVEROBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

servlookup: $(SERVLOOKUPOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

sockname: $(SOCKNAMEOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

sockpair: $(SOCKPAIROBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

speed: $(SPEEDOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

speed2: $(SPEED2OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

speedd: $(SPEEDDOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

tcpcl: $(TCPCLOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

tcpsv: $(TCPSVOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

udpclnt: $(UDPCLNTOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)

udpserv: $(UDPSERVOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^ $(LIBS)


include $(top_srcdir)/DEPENDENCIES
