CC = gcc
CFLAGS = -O2 -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: tiny cgi

tiny.o: tiny.c 
	$(CC) $(CFLAGS) -c tiny.c 

sio.o: sio.c
	$(CC) $(CFLAGS) -c sio.c 

interface.o: interface.c
	$(CC) $(CFLAGS) -c interface.c 

tiny: tiny.o sio.o interface.o
	$(CC) $(CFLAGS) tiny.o sio.o interface.o -o tiny $(LIB)

cgi:
	(cd cgi-bin; make)

clean:
	rm -f *.o tiny *~
	(cd cgi-bin; make clean)

