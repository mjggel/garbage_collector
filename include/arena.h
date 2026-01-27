#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct Block {
    size_t size;
    struct Block* next;
} Block;

typedef struct
{
    void* start;
    size_t size;
    Block* free_list;
} GC_Arena;


#define TAG_MARK 0x1
#define TAG_FREE 0x2
#define TAG_MASK 0x7

#define UNTAG_BLOCK(p) ((Block*)((uintptr_t)(p) & ~TAG_MASK))

void gc_init(size_t size);

#endif