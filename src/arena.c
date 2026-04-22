#include <sys/mman.h>
#include <setjmp.h>
#include <stdio.h>
#include "arena.h"
#include <signal.h>

#define MIN_WORTH_SIZE 8

static GC_Arena global_arena;

void segv_handler(int sig, siginfo_t *si, void *unused) {

    (void)sig;
    (void)unused;
    
    uintptr_t fault_addr = (uintptr_t)si->si_addr;
    uintptr_t start = (uintptr_t)global_arena.start;
    uintptr_t end = start + global_arena.size;

    if (fault_addr >= start && fault_addr < end) {
        
        size_t page_idx = (fault_addr - start) / PAGE_SIZE;
        
        global_arena.dirty_pages[page_idx] = 1;

        void* page_start = (void*)(start + (page_idx * PAGE_SIZE));
        mprotect(page_start, PAGE_SIZE, PROT_READ | PROT_WRITE);
        
        
        printf("  [Write Barrier] Acesso detectado na página %zu! Liberando acesso...\n", page_idx);
        
    } else {
        fprintf(stderr, "\n[FATAL] Segmentation Fault real no endereço: %p\n", (void*)fault_addr);
        signal(SIGSEGV, SIG_DFL); 
    }
}

void gc_init(size_t size, void* stack_top) {

    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

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

   

    global_arena.num_pages = size / PAGE_SIZE;

    global_arena.dirty_pages = (uint8_t*)mmap(
        NULL,
        global_arena.num_pages,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    for(size_t i = 0; i < global_arena.num_pages; i++) {
        global_arena.dirty_pages[i] = 0;
    }

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segv_handler;
    
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("Error installing signal handler");
    }
    

    Block* first = (Block*)ptr;

    first->size = (uint32_t)(size - sizeof(Block));
    first->next = (Block*)((uintptr_t) NULL | TAG_FREE);
    first->age = 0;

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
                
                next_block->size = (uint32_t)(split - sizeof(Block));
                next_block->age = 0;

                current->size = (uint32_t)size;
                current->age = 0;

                Block* old_next = GET_NEXT(current);

                next_block->next = (Block*) ( (uintptr_t) old_next | TAG_FREE);  

                current->next = (Block*) ( (uintptr_t) next_block | TAG_FREE); 

            }

            if(current->size >= size) {

                REMOVE_TAG(current, TAG_FREE);
                current->age = 0;
                return (void*) (current + 1);
            }

        }

        current = GET_NEXT(current);
    }

    return NULL;
}

void* get_rsp() {
    void* rsp;
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

    ptr->next = (Block*) ((uintptr_t)ptr->next | TAG_MARK);

    printf("Marking block at %p (Age: %u)\n", ptr, ptr->age);

    uintptr_t* start = (uintptr_t*)p;
    uintptr_t* end = (uintptr_t*)((char*)p + ptr->size);

    for(uintptr_t* current = start; current < end; current++) {
        if(is_pointer(*current)) {
            printf("  [MARKING CHILD] Addr: %p | Value: 0x%lx (Points to Arena)\n", current, *current);
            gc_mark((void*)*current);
        }
    }
    
}   

void gc_sweep(){

    Block* current = global_arena.list;

    while(current != NULL) {
    restart_scan:;

        Block* next_block = UNTAG_BLOCK(current->next);

        uintptr_t tags = (uintptr_t)current->next & TAG_MASK;

        //if was MARKED tags: [0x...001]
        //if was NOT MARKED AND NOT FREE tags: [0x...000] -- DEAD
        //if was FREE tags: [0x...011] -- FREE

        uintptr_t new_tags = (~tags & TAG_MARK) << 1;

        //if was MARKED --> ((~[0x...001])->[0x...110] & [0x...001]) -> [0x...000] -- LIVE
        //if was NOT MARKED AND NOT FREE --> ((~[0x...000])->[0x...111] & [0x...001]) -> [0x...001<<] -> [0x...010] -- FREE
        //if was FREE --> ((~[0x...010])->[0x...101] & [0x...001]) -> [0x...001<<] -> [0x...010] -- FREE

        uint32_t is_survivor = !(new_tags & TAG_FREE);

        //since new_tags is eiter 0 or the TAG_FREE bit
        //if is FREE --> is_survivor = 0 -> !([0x...010] & [0x...010] == ***1***)
        //if is LIVE --> is_survivor = 1 -> !([0x...000] & [0x...010] == ***0***)

        current->age = (current->age + is_survivor) * is_survivor;

        current->next = (Block*) ((uintptr_t)next_block | new_tags);

        if((new_tags & TAG_FREE) && next_block != NULL) {
            
            uintptr_t next_block_tag = (uintptr_t)next_block->next & TAG_MASK;


            if(!(next_block_tag & TAG_MARK)) {
            
                current->size += sizeof(Block) + next_block->size;
                
                current->age = 0;

                uintptr_t next_next_addr = (uintptr_t)next_block->next & ~TAG_MASK;

                current->next = (Block*)(next_next_addr | new_tags);

                goto restart_scan;
            }
        }

        
        current = GET_NEXT(current);
    }
    
}

void gc_collect(){
    jmp_buf env;
    setjmp(env);

    uintptr_t* current_stack = (uintptr_t*)get_rsp();
    uintptr_t* stack_top = (uintptr_t*) global_arena.stack_top;

    printf("\n--- GC START: Scanning Stack ---\n");

    for(size_t i = 0; i < global_arena.num_pages; i++) {
        void* page_start = (char*)global_arena.start + (i * PAGE_SIZE);
        mprotect(page_start, PAGE_SIZE, PROT_READ | PROT_WRITE);
    }

    while(current_stack < stack_top){
        if(is_pointer(*current_stack)) {
            gc_mark((void*)*current_stack);
        }
        current_stack++;
    }

    for(size_t i = 0; i < global_arena.num_pages; i++) {
        if(global_arena.dirty_pages[i]) {
            uintptr_t* page_start = (uintptr_t*)((char*)global_arena.start + (i * PAGE_SIZE));
            uintptr_t* page_end = (uintptr_t*)((char*)page_start + PAGE_SIZE);

            for(uintptr_t* ptr = page_start; ptr < page_end; ptr++) {
                if(is_pointer(*ptr)) {
                    gc_mark((void*)*ptr);
                }
            }
        }
    }

    gc_sweep();

    for(size_t i = 0; i < global_arena.num_pages; i++) {
        global_arena.dirty_pages[i] = 0;
        
        void* page_start = (char*)global_arena.start + (i * PAGE_SIZE);
        mprotect(page_start, PAGE_SIZE, PROT_READ);
    }

    printf("--- GC END ---\n");
}

void gc_destroy() {
    if (global_arena.start != NULL) {
        if(global_arena.dirty_pages != NULL) {
            munmap(global_arena.dirty_pages, global_arena.num_pages);
            global_arena.dirty_pages = NULL;
        }
        
        munmap(global_arena.start, global_arena.size);
        global_arena.start = NULL;
        global_arena.list = NULL;
        global_arena.size = 0;
        printf("Arena destroyed and memory released.\n");
    }
}

void debug_heap() {
    printf("\n--- HEAP LAYOUT ---\n");
    Block* current = global_arena.list;
    int i = 0;
    while (current != NULL) {
        uintptr_t next_addr = (uintptr_t)UNTAG_BLOCK(current->next);
        uintptr_t tags = (uintptr_t)current->next & TAG_MASK;
        
        char* status = (tags & TAG_MARK) ? "[MARKED/LIVE]" : 
                       (tags & TAG_FREE) ? "[FREE]" : "[USED/DEAD]";

        printf("Block %d: %p | Size: %u | Status: %s\n", 
               i++, (void*)current, current->size, status);

        current = (Block*)next_addr;
    }
    printf("-------------------\n");
}