# Makefile for GNU CC
# using 16 bit intsD

# makevars : set variables for make

# This is where the distribution lives
BUILDDIR=.

# this is where the xfs has to be installed
MINTDIR=/c/multitos

# this is where the mount command has to be installed
ETCDIR=/usr/etc

# this is where the mountd and nfsd have to be installed
USRETCDIR=/usr/etc

# Compiler etc.
CC = gcc
CFLAGS = -mshort -Wall -O2 -fomit-frame-pointer -I$(BUILDDIR)/lib
AS = as
LD = gcc
LDFLAGS = -mshort
LIBS = -L$(BUILDDIR)/lib -lrpc16 -lsocket
AR = ar
ARFLAGS = crs


all  : librpc16.a
install : librpc16.a


OB0 = thread.o svc_main.o pmap_set.o pmap_get.o pmap_xdr.o 
OB1 = svc_main.o svc_auth.o svc.o clnt.o rpc_xdr.o xdr.o


thread.o    : thread.c thread.h

pmap_set.o  : pmap_set.c pmap.h clnt.h 
pmap_get.o  : pmap_get.c pmap.h clnt.h
pmap_xdr.o  : pmap_xdr.c pmap.h xdr.h

svc_main.o  : svc_main.c svc.h rpc.h xdr.h
svc.o       : svc.c svc.h rpc.h xdr.h
svc_auth.o  : svc_auth.c auth_unix.h svc.h rpc.h xdr.h

clnt.o      : clnt.c clnt.h

rpc_xdr.o   : rpc_xdr.c rpc.h xdr.h
xdr.o       : xdr.c xdr.h


librpc16.a: $(OB0) $(OB1)
	$(AR) $(ARFLAGS) $@ $(OB0) $(OB1)
	ranlib $@

clean :
	$(RM) -f *.o
