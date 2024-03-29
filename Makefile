.PHONY: bin test debug tar assets clean

ARCH_NAME=LABSO2015_Progetto_multiplayer_game_165907_164492
CFLAGS=-Iinclude/ -std=gnu99 -pthread -Wall -Werror -pedantic -pedantic-errors
DFLAGS=-g -DDEBUG

default:
	@echo "Available commands:"
	@echo " - bin: make all binaries"
	@echo " - server: make server library"
	@echo " - client: make client library"
	@echo " - tar: make source archive"
	
bin: clean
	mkdir -p bin lib
	gcc $(CFLAGS) -c src/client/client.c -o lib/client.o
	gcc $(CFLAGS) -c src/server/server.c -o lib/server.o
	gcc $(CFLAGS) lib/server.o lib/client.o src/main.c -o bin/main.out

test: clean
	mkdir -p bin lib assets
	gcc $(CFLAGS) -DTEST -c src/client/client.c -o lib/client.o
	gcc $(CFLAGS) -DTEST -c src/server/server.c -o lib/server.o
	gcc $(CFLAGS) -DTEST lib/server.o lib/client.o src/main.c -o bin/main.out

debug: clean
	mkdir -p bin lib
	gcc $(CFLAGS) $(DFLAGS) -c src/client/client.c -o lib/client.o
	gcc $(CFLAGS) $(DFLAGS) -c src/server/server.c -o lib/server.o
	gcc $(CFLAGS) $(DFLAGS) lib/server.o lib/client.o src/main.c -o bin/main.out

tar: clean
	mkdir $(ARCH_NAME)
	cp -R src include Makefile $(ARCH_NAME)
	tar zcvf $(ARCH_NAME).tar.gz $(ARCH_NAME)
	rm -rf $(ARCH_NAME)

assets:
	mkdir -p assets
	gcc -Wall src/maketests.c -o bin/maketests.out
	bin/maketests.out 123 2 assets/server.test assets/client1.test assets/client2.test

clean:
	rm -rf bin lib assets /tmp/sisop-* $(ARCH_NAME).tar.gz
