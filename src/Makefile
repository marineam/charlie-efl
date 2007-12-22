CC	= gcc
CFLAGS  = -ggdb -Wall -std=gnu99
LIBS    = -lecore -lecore_evas -ledje
LLIBS	= lib/libmpdclient/libmpdclient.o lib/e/e_box.o
HEAD	= layout.h main.h mpdclient.h music.h scrollbox.h ../config.h
SRC	= layout.c main.c mpdclient.c music.c scrollbox.c
OBJ	= $(SRC:.c=.o)

all: main

.c.o: $(HEAD)
	$(CC) $(CFLAGS) -c $*.c

lib-stamp:
	$(MAKE) CFLAGS="$(CFLAGS)" -C lib
	touch lib-stamp

main: $(OBJ) $(HEAD) lib-stamp
	$(CC) $(CFLAGS) $(LIBS) -o main $(OBJ) $(LLIBS)

clean:
	$(MAKE) -C lib clean
	$(RM) *.o main *~ lib-stamp

