DEBUG_FLAGS=-g -DDEBUG
WARNING_FLAGS=-Wall -Werror -pedantic -pedantic-errors
STD_FLAGS=-Iinclude/ -std=gnu99
CFLAGS=$(STD_FLAGS) $(WARNING_FLAGS) $(DEBUG_FLAGS)
ARCH_NAME=LABSO2015_Progetto_multiplayer_game_165907_164492.tar.gz
all: clean bin

bin: src/* include/*
	gcc $(CFLAGS) src/server/server.c -o bin/server.out
	gcc $(CFLAGS) src/client/client.c -o bin/client.out

clean:
	rm -rf bin/* /tmp/sisop-*

consegna:
	tar zcvf $(ARCH_NAME) src include Makefile
