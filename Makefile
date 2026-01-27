CC = gcc
CFLAGS = -Wall -Wextra -g3 -O0 -fno-stack-protector -Iinclude

all:
	$(CC) $(CFLAGS) src/arena.c src/main.c -o gc_project

clean:
	rm -f gc_project