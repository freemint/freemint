# Makefile for gcc using 16 bit integers

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

# standard utilities
CP = /bin/cp
RM = /bin/rm
MV = /bin/mv
INSTALL = /bin/install -m 755


all:  mount_nfs
install : mount_nfs
	$(INSTALL) mount_nfs $(ETCDIR)

MNTOBJ0 = mount.o mntent.o 
MNTOBJ1 = nfsmnt.o mount_xdr.o

mount_nfs  : $(MNTOBJ0) $(MNTOBJ1)
		$(LD) $(LDFLAGS) -o mount_nfs $(MNTOBJ0) $(MNTOBJ1) $(LIBS)
		strip mount_nfs

mount.o    : mount.c mntent.h common.h 
mntent.o   : mntent.c mntent.h

nfsmnt.o   : nfsmnt.c common.h ../nfs/nfs_xdr.h

mount_xdr.o : mount_xdr.c mount.h


clean :
	$(RM) -f *.o
