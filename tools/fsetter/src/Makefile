#
# Makefile for fsetter
#
TARGET = fsetter.app

CC = gcc -pipe
DEFINITIONS = 
WARN = -Wall -Wmissing-prototypes -Winline
OPT = -fomit-frame-pointer -O2
CFLAGS = -I/usr/GEM/include $(MODEL) $(OPT) $(DEFINITIONS) $(WARN)
LDFLAGS =
LIBS= -L/usr/GEM/lib -lcflib -lgemx -lgem

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
	fsetter.c \
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

-include $(DEPENDENCIES) /dev/null
