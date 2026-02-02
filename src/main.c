#include <stdio.h>
#include "arena.h"
int main() {
    void* stack_top;
    stack_top = (void*)&stack_top;
    
    gc_init(4096, stack_top);

    printf("\n>>> TEST 1: NORMAL ALOCATION <<<\n");

    void* ptr = gc_alloc(64);
    printf("User pointer: %p", ptr);

    gc_collect();


    printf("\n>>> TEST 2: TRASH IN THE STACK <<<\n");
    gc_alloc(64);

    gc_collect();


    return 0;
}
