#
# Makefile for flop_raw.xdd
#

# compiler settings
CC = gcc -pipe
AS = $(CC) -c
LD = $(CC)
CP = cp
RM = rm -f


OPTS =	-fomit-frame-pointer -O3

WARN =	-Wall \
	-Wmissing-prototypes \
	-Wshadow \
	-Wcast-qual \
	-Waggregate-return

CFLAGS = -mshort $(WARN) $(OPTS)
LDFLAGS = -lsocket16

TARGET = gluestik.prg
OBJS = \
	gs_config.o \
	gs_func.o \
	gs_mem.o \
	gs_stik.o \
	main.o

$(TARGET): $(OBJS)
	$(LD) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	flags --mglobal-memory $@

boot:
	$(CP) $(TARGET) /c/multitos

clean:
	$(RM) $(OBJS)

distclean:
	$(MAKE) clean;
	$(RM) *~

# dependencies
.depend: *.h
	$(CC) -MM $(CPPFLAGS) $(OBJS:.o=.c) > .depend

include .depend
