CFLAGS = -Wall -ggdb
SOURCES = example.c libmpdclient.c

all: libmpdclient.o

libmpdclient.o: libmpdclient.c
	$(CC) $(CFLAGS) -c libmpdclient.c

example: $(SOURCES)

clean:
	rm -f *.o example
