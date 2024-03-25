CC		= gcc
CFLAGS	= -Wall -g -D_GNU_SOURCE -fPIC -ldl
PROGS	= libsolver.so


%.o: %.c
	$(CC) -c $< $(CFLAGS)

maze: maze.o libsolver.so libmaze.so
	$(CC) -o $@ $^ 2> /dev/null

libsolver.so: libsolver.o 
	$(CC) -o $@ -shared $^  2> /dev/null

libmaze.so: libmaze.o
	$(CC) -o $@ -shared $^  2> /dev/null

test:
	LD_LIBRARY_PATH=`pwd` LD_PRELOAD=./libsolver.so ./server_maze

run:
	LD_LIBRARY_PATH=`pwd` ./server_maze

cleanup:
	rm -f *.o

clean:
	rm -f *.so *.o maze