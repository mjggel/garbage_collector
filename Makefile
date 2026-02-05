CC = gcc
CFLAGS = -Wall -Wextra -g3 -O0 -fno-stack-protector -Iinclude

# Alvos
all: gc_project test_suite

# Compila o executável principal
gc_project: src/main.c src/arena.c
	$(CC) $(CFLAGS) src/main.c src/arena.c -o gc_project

# Compila o executável de testes (Linka o teste com a arena)
test_suite: tests/stress_test.c src/arena.c
	$(CC) $(CFLAGS) tests/stress_test.c src/arena.c -o test_suite

clean:
	rm -f gc_project test_suite