TARGET = mkfatfs
SBINDIR  = /sbin
MANDIR  = /usr/man

CC = gcc -pipe
DEFINITIONS = 
WARN =	-Wall \
	-Wmissing-prototypes \
	-Wshadow \
	-Wcast-qual \
	-Waggregate-return \
	-Wparentheses
OPT = -O
CFLAGS = $(MODEL) $(OPT) $(DEFINITIONS) $(WARN)
RANLIB = ranlib
CRLF = crlf -s

#
# files
#
CFILES = \
	mkfatfs.o \
	xhdi.c

OBJS =	$(CFILES:.c=.o)

#
# main target
#
$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

clean:
	rm -f $(TARGET) $(OBJS)

distclean:
	rm -f $(TARGET) $(OBJS)
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
