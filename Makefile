CFLAGS=-Wall -Iinclude/ -g -pthread -std=gnu99

all: clean server client

server: src/server/* include/*
	gcc $(CFLAGS) src/server/server.c -o bin/server.out

client: src/client/* include/*
	gcc $(CFLAGS) src/client/client.c -o bin/client.out

clean:
	rm -rf bin/* /tmp/sisop-*
