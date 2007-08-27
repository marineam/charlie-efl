CFLAGS  = -g -Wall
LIBS    = -lecore -lecore_evas -ledje -leet
HEAD	= main.h layout.h music.h e_box.h e_table.h
OBJ	= layout.o music.o e_box.o e_table.o

all: main

.c.o: $(HEAD)
	$(CC) $(CFLAGS) -c $*.c

main: $(OBJ) $(HEAD) main.c
	$(CC) $(CFLAGS) $(LIBS) -o main main.c $(OBJ)

clean:
	$(RM) *.o main *~

