CC= gcc
CFLAGS= -m68020-60 -Wall -O3 -fomit-frame-pointer -fno-defer-pop\
	-fno-inline-functions -fexpensive-optimizations
LFLAGS=
LIBS=
TARGET= memspeed

OBJS= memspeed.o blockcopy.o

all: $(TARGET)
	fixstk 2k $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

spp:
	$(CC) $(CFLAGS) -S -c $(TARGET).c

clean:
	rm -f *.o

veryclean:
	rm -f *.o $(TARGET)
