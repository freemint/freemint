#
# Makefile for crypt I/O initialization
#
TARGET = crypto

CC = gcc -pipe
DEFINITIONS = 
WARN = -Wall -Wmissing-prototypes -Winline
OPT = -fomit-frame-pointer -O2
CFLAGS = -mshort $(MODEL) $(OPT) $(DEFINITIONS) $(WARN)
RANLIB = ranlib
CRLF = crlf -s

#
# C source files
#
%.o: %.c
	@echo "$(CC) $(CFLAGS) -c $< -o $@"; \
	$(CC) -Wp,-MD,.deps/$(@:.o=.P) $(CFLAGS) -c $< -o $@

#
# files
#
CFILES = \
	blowfish.c \
	crypt.c \
	io.c \
	main.c \
	md5.c \
	xhdi.c

OBJS =	$(CFILES:.c=.o)

DEPENDENCIES = $(addprefix ./.deps/, $(patsubst %.c,%.P,$(CFILES)))

#
# main target
#
$(TARGET): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)


#
# macros for cleaning up
#
clean:
	$(RM) $(OBJS) $(TARGET)

distclean:
	$(MAKE) clean
	$(RM) -r .deps
	$(RM) *~


#
# dependencies
#
DEPS_MAGIC := $(shell mkdir ./.deps > /dev/null 2>&1 || :)

-include $(DEPENDENCIES)
