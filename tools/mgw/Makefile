#
# Makefile for 
#

# additional includes
INCLUDE = 

# compiler settings
CC = gcc
AS = $(CC)
LD = $(CC)
STRIP = strip
CP = cp
RM = rm -f

#CPU = -m68030
#CPU = -m68040

OPTS =  -fomit-frame-pointer \
	-O $(CPU)

# the -mshort flag is mandatory
CFLAGS = -g $(INCLUDE) -Wall -Wmissing-prototypes -mshort $(OPTS)
LDFLAGS = -lsocket16

TARGET = mgw.prg
OBJS = main.o mgw.o

all: $(TARGET) $(TARGET2) $(TARGET3)

$(TARGET): $(OBJS)
	$(LD) -mshort $(OPTS) -o $@ $(OBJS) $(LDFLAGS)
	flags --mglobal-memory $@

boot:
	$(CP) $(TARGET) /c/multitos

clean:
	$(RM) $(OBJS)

distclean:
	$(MAKE) clean;
	$(RM) *~
