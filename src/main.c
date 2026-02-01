#include <stdio.h>
#include "arena.h"
int main() {
    void* stack_top;
    stack_top = &stack_top;
    
    gc_init(4096, stack_top);

    gc_alloc();

    return 0;
}
