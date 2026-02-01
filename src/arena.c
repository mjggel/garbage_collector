#include <sys/mman.h>
#include <stdio.h>
#include "arena.h"

#define MIN_WORTH_SIZE 8

static GC_Arena global_arena;

void gc_init(size_t size, void* stack_top) {


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
    global_arena.stack_top = stack_top;


    Block* first = (Block*)ptr;


    first->size = size - sizeof(Block);

    first->next = (Block*)((uintptr_t) NULL | TAG_FREE);

    global_arena.list = first;

    printf("Arena initialized at %p with %zu bytes\n", ptr, size);
}

void* gc_alloc(size_t size) {

    size = ALIGN(size);

    Block* current = global_arena.list;

    while(current != NULL) {

        if(HAS_TAG(current, TAG_FREE)) {

            size_t split = current->size - size;

            if(split >= (sizeof(Block) + MIN_WORTH_SIZE) ) {



                uintptr_t current_addr = (uintptr_t)current;
                
                Block* next_block = (Block*)((uintptr_t)current_addr + sizeof(Block) + size);
                
                next_block->size = split - sizeof(Block);

                current->size = size;

                Block* old_next = GET_NEXT(current);

                next_block->next = (Block*) ( (uintptr_t) old_next | TAG_FREE);  

                current->next = (Block*) ( (uintptr_t) next_block | TAG_FREE); 

            }

            if(current->size >= size) {

                REMOVE_TAG(current, TAG_FREE);

                return (void*) (current + 1);
            }

        }

        current = GET_NEXT(current);
    }

    return NULL;
}

void* get_rsp() {
    void * rsp;
    asm("mov %%rsp, %0" : "=r"(rsp));
    return rsp;

}

void gc_collect(){
    uintptr_t* current_stack = (uintptr_t*)get_rsp();
    uintptr_t* stack_top = (uintptr_t*) global_arena.stack_top;

    while(current_stack < stack_top){
        printf("stack value: 0x%lx", *current_stack);

        current_stack++;
    }
}
