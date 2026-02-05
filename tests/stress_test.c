#include <stdio.h>
#include <assert.h>
#include "arena.h"

struct Node {
    struct Node* next;
    int value;
};

void test_coalescing() {
    printf("\n=== TEST 1: COALESCING (The Fusion) ===\n");
    
    void* a = gc_alloc(64);
    void* b = gc_alloc(64);
    void* c = gc_alloc(64);
    printf("Allocated A: %p, B: %p, C: %p\n", a, b, c);
    
    debug_heap();

    printf(">> Dropping refs to A and B...\n");
    a = NULL;
    b = NULL;
    
    gc_collect();
    
    printf(">> After Collect (Expect A+B merged, C alive):\n");
    debug_heap();

    printf(">> Allocating D (128 bytes)...\n");
    void* d = gc_alloc(128);
    printf("Allocated D: %p\n", d);
    debug_heap();

    if (d < c) {
        printf("[SUCCESS] D took the place of A and B!\n");
    } else {
        printf("[FAIL] D was allocated at the end. Coalescing failed.\n");
    }
}

void test_cycle() {
    printf("\n=== TEST 2: CIRCULAR REFERENCE (The Ouroboros) ===\n");
    
    struct Node* n1 = (struct Node*)gc_alloc(sizeof(struct Node));
    struct Node* n2 = (struct Node*)gc_alloc(sizeof(struct Node));
    
    n1->next = n2;
    n2->next = n1;
    n1->value = 1;
    n2->value = 2;
    
    printf("Created Cycle: N1(%p) <-> N2(%p)\n", n1, n2);
    
    printf(">> Dropping Roots...\n");
    n1 = NULL;
    n2 = NULL;
    
    gc_collect();
    
    printf(">> After Collect (Expect Cycle DEAD/FREE):\n");
    debug_heap();
}

int main() {
    void* stack_top = (void*)&stack_top;
    gc_init(4096, stack_top);

    test_coalescing();
    test_cycle();

    return 0;
}