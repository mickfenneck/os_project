DEBUG_FLAGS=-g -DDEBUG
WARNING_FLAGS=-Wall -Werror -pedantic -pedantic-errors
STD_FLAGS=-Iinclude/ -std=gnu99
CFLAGS=$(STD_FLAGS) $(WARNING_FLAGS) $(DEBUG_FLAGS)
ARCH_NAME=LABSO2015_Progetto_multiplayer_game_165907_164492.tar.gz


all: clean server client bin

bin: src/main.c lib/server.o lib/client.o include/*
	gcc $(CFLAGS) lib/server.o lib/client.o src/main.c -o bin/main.out

server: src/server.c include/*
	gcc $(CFLAGS) -c src/server.c -o lib/server.o

client: src/client.c include/*
	gcc $(CFLAGS) -c src/client.c -o lib/client.o

clean:
	rm -rf bin/* lib/* /tmp/sisop-*

tar: clean
	tar zcvf $(ARCH_NAME) src include Makefile lib bin
