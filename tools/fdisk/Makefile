TARGETS = sfdisk fdisk
SBINDIR  = /sbin
MANDIR  = /usr/man

CC = gcc -pipe
DEFINITIONS = 
WARN = -Wall -Wparentheses -Winline
#OPT = -fomit-frame-pointer -O2
CFLAGS = $(MODEL) $(OPT) $(DEFINITIONS) $(WARN)
RANLIB = ranlib
CRLF = crlf -s

#
# files
#
CFILES = \
	fdisk.o \
	sfdisk.c \
	xhdi.c

OBJS =	$(CFILES:.c=.o)

SFDISKOBJS = sfdisk.o xhdi.o
FDISKOBJS = fdisk.o xhdi.o

#
# main target
#
all: $(TARGETS)

sfdisk: $(SFDISKOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(SFDISKOBJS) $(LIBS)

fdisk: $(FDISKOBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(FDISKOBJS) $(LIBS)

clean:
	rm -f $(TARGETS) $(OBJS)

distclean:
	rm -f $(TARGETS) $(OBJS)
	rm -f *~
	rm -rf .deps

install:
	install -d -m 755 $(SBINDIR)
	install -m 755 $(TARGET) $(SBINDIR)
	install -d -m 755 $(MANDIR)/man8
	install -m 644 $(TARGET).8 $(MANDIR)/man8

#
# C source files
#
%.o: %.c
	@echo "$(CC) $(CFLAGS) -c $< -o $@"; \
	$(CC) -Wp,-MD,.deps/$(@:.o=.P) $(CFLAGS) -c $< -o $@

DEPS_MAGIC := $(shell mkdir ./.deps > /dev/null 2>&1 || :)
DEPENDENCIES = $(addprefix ./.deps/, $(patsubst %.c,%.P,$(CFILES)))

-include $(DEPENDENCIES) /dev/null
