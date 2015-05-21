ARCH_NAME=LABSO2015_Progetto_multiplayer_game_165907_164492
CFLAGS_STD=-Iinclude/ -std=gnu99 -pthread -Wall -Werror -pedantic -pedantic-errors
ifeq ($(debug),true)
	CFLAGS=$(CFLAGS_STD) -g -DDEBUG
else
	CFLAGS=$(CFLAGS_STD)
endif

default:
	@echo "Available commands:"
	@echo " - bin: make all binaries"
	@echo " - server: make server library"
	@echo " - client: make client library"
	@echo " - tar: make source archive"
	
bin: clean client server
	[ -d bin ] || mkdir bin
	gcc $(CFLAGS) lib/server.o lib/client.o src/main.c -o bin/main.out

server:
	[ -d lib ] || mkdir lib
	gcc $(CFLAGS) -c src/server/server.c -o lib/server.o

client:
	[ -d lib ] || mkdir lib
	gcc $(CFLAGS) -c src/client/client.c -o lib/client.o

clean:
	rm -rf bin lib /tmp/sisop-* $(ARCH_NAME).tar.gz

tar: clean
	mkdir $(ARCH_NAME)
	cp -R 	src include Makefile $(ARCH_NAME)
	tar zcvf $(ARCH_NAME).tar.gz $(ARCH_NAME)
	rm -rf $(ARCH_NAME)
