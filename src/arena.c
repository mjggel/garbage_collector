#include <sys/mman.h>
#include <setjmp.h>
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

int is_pointer(uintptr_t p) {
    uintptr_t start = (uintptr_t) global_arena.start;
    uintptr_t end = start + (uintptr_t) global_arena.size;

    return (p > start && p < end);
}

Block* get_block_header(void* p) {
    return (Block*) ((char*)p - sizeof(Block));
}

void gc_mark(void* p) {
    Block* ptr = get_block_header(p);

    if(HAS_TAG(ptr, TAG_MARK)){
        return;
    }
    printf("Marking block at %p\n", ptr);
    ptr->next = (Block*) ((uintptr_t)ptr->next | TAG_MARK);
}

void gc_collect(){
    jmp_buf env;
    setjmp(env);

    uintptr_t* current_stack = (uintptr_t*)get_rsp();
    uintptr_t* stack_top = (uintptr_t*) global_arena.stack_top;

    printf("\n--- GC START: Scanning Stack ---\n");
    printf("Stack Bottom: %p | Stack Top: %p\n", current_stack, stack_top);

    while(current_stack < stack_top){
        if(is_pointer(*current_stack)) {
            printf("[FOUND ROOT] Addr: %p | Value: 0x%lx (Points to Arena)\n", current_stack, *current_stack);
            gc_mark((void*)*current_stack);
        }
        current_stack++;
    }
    
    printf("--- GC END ---\n");
}
