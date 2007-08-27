CFLAGS  = -g -Wall
HEAD	= *.h
OBJ	= e_box.o

all: $(OBJ)

.c.o: $(HEAD)
	$(CC) $(CFLAGS) -c $*.c

clean:
	$(RM) *.o

