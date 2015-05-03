DEBUG_FLAGS=-g -DDEBUG
CFLAGS=-Wall -Werror -Iinclude/ -std=gnu99 -pedantic -pedantic-errors

all: clean server client

server: src/server/* include/*
	gcc $(CFLAGS) $(DEBUG_FLAGS) src/server/server.c -o bin/server.out

client: src/client/* include/*
	gcc $(CFLAGS) $(DEBUG_FLAGS) src/client/client.c -o bin/client.out

clean:
	rm -rf bin/* /tmp/sisop-*
