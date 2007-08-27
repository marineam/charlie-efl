CFLAGS  = -g -Wall
LIBS    = -lecore -lecore_evas -ledje -leet
LLIBS	= lib/libmpdclient/libmpdclient.o
HEAD	= *.h
OBJ	= layout.o music.o mpdclient.o e_box.o

all: main

.c.o: $(HEAD)
	$(CC) $(CFLAGS) -c $*.c

lib-stamp:
	$(MAKE) CFLAGS="$(CFLAGS)" -C lib
	touch lib-stamp

main: $(OBJ) $(HEAD) lib-stamp main.c
	$(CC) $(CFLAGS) $(LIBS) -o main main.c $(OBJ) $(LLIBS)

clean:
	$(MAKE) -C lib clean
	$(RM) *.o main *~ lib-stamp

