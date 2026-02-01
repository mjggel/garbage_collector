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
    void* stack_top;
    size_t size;
    Block* list;
    
} GC_Arena;
    

#define TAG_MARK 0x1
#define TAG_FREE 0x2
#define TAG_MASK 0x7

#define UNTAG_BLOCK(p) ((Block*)((uintptr_t)(p) & ~TAG_MASK))

#define HAS_TAG(p, tag) (((uintptr_t)(p->next) & (tag)) != 0)

#define REMOVE_TAG(p, tag) ((p->next) = (Block*)((uintptr_t)(p->next) & ~(tag)))

#define GET_NEXT(p) (UNTAG_BLOCK((p)->next))

#define ALIGN(p) ((p + TAG_MASK) & ~TAG_MASK)

void gc_init(size_t size);

#endif