ARCH_NAME=LABSO2015_Progetto_multiplayer_game_165907_164492
CFLAGS=-Iinclude/ -std=gnu99 -pthread -Wall -Werror -pedantic -pedantic-errors

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
	mkdir -p bin lib
	gcc $(CFLAGS) -DTEST -c src/client/client.c -o lib/client.o
	gcc $(CFLAGS) -DTEST -c src/server/server.c -o lib/server.o
	gcc $(CFLAGS) -DTEST lib/server.o lib/client.o src/main.c -o bin/main.out

debug: clean
	mkdir -p bin lib
	gcc $(CFLAGS) -g -DDEBUG -c src/client/client.c -o lib/client.o
	gcc $(CFLAGS) -g -DDEBUG -c src/server/server.c -o lib/server.o
	gcc $(CFLAGS) -g -DDEBUG lib/server.o lib/client.o src/main.c -o bin/main.out

tar: clean
	mkdir $(ARCH_NAME)
	cp -R src include Makefile $(ARCH_NAME)
	tar zcvf $(ARCH_NAME).tar.gz $(ARCH_NAME)
	rm -rf $(ARCH_NAME)

clean:
	rm -rf bin lib assets /tmp/sisop-* $(ARCH_NAME).tar.gz
