#
# Makefile for minixfs checker
#
TARGET = fsck.minix

SHELL = /bin/sh
SUBDIRS = 

srcdir = .
top_srcdir = ../..
subdir = fsck

default: all

include $(top_srcdir)/CONFIGVARS
include $(top_srcdir)/RULES
include $(top_srcdir)/PHONY

all-here: $(TARGET)

# default overwrites
INCLUDES = -I$(top_srcdir)/IO

# default definitions
OBJS = $(COBJS:.c=.o) fsck1.o
LIBS += $(LIBIO)
GENFILES = $(TARGET)

$(TARGET): $(OBJS) $(LIBIOSTMP)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

fsck1.o: fsck.c fsck.o
	@echo "$(CC) -DV1 $(CFLAGS) -c $< -o $@"; \
	$(CC) -Wp,-MD,.deps/$(@:.o=.P) -DV1 $(CFLAGS) -c $< -o $@


include $(top_srcdir)/DEPENDENCIES
