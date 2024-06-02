CC=gcc
CFLAGS=-I. -std=gnu99
DEPS=
OBJ=server.o
USERID=123456789

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: server
server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

start:
	./server

debug:
	$(CC) server.c -g && gdb a.out
clean:
	rm -rf *.o server *.tar.gz *.out

dist: tarball
tarball: clean
	tar -cvzf /tmp/$(USERID).tar.gz --exclude=./.vagrant . && mv /tmp/$(USERID).tar.gz .
