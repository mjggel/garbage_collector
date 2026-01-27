#include <sys/mman.h>
#include <stdio.h>
#include "arena.h"

static GC_Arena global_arena;

void gc_init(size_t size) {


    void* ptr = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (ptr == MAP_FAILED) {
        perror("MMAP FAILED!!!");
        return;
    }

    global_arena.start = ptr;
    global_arena.size = size;

    Block* first = (Block*)ptr;


    first->size = size - sizeof(Block);

    first->next = (Block*)((uintptr_t) NULL | TAG_FREE);

    global_arena.free_list = first;

    printf("Arena initialized at %p with %zu bytes\n", ptr, size);
}