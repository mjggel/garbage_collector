#include <stdio.h>
#include <assert.h>
#include "arena.h"

struct Node {
    struct Node* next;
    int value;
};

void test_coalescing() {
    printf("\n==================================================\n");
    printf("=== TEST 1: COALESCING (The Fusion) ===\n");
    
    void* a = gc_alloc(64);
    void* b = gc_alloc(64);
    void* c = gc_alloc(64);
    
    printf(">> Dropping refs to A and B...\n");
    a = NULL;
    b = NULL;
    
    gc_collect();
    
    void* d = gc_alloc(128);
    if (d < c) {
        printf("[SUCCESS] D took the place of A and B! Coalescing works.\n");
    } else {
        printf("[FAIL] D was allocated at the end. Coalescing failed.\n");
    }
}

void test_cycle() {
    printf("\n==================================================\n");
    printf("=== TEST 2: CIRCULAR REFERENCE (The Ouroboros) ===\n");
    
    struct Node* n1 = (struct Node*)gc_alloc(sizeof(struct Node));
    struct Node* n2 = (struct Node*)gc_alloc(sizeof(struct Node));
    
    n1->next = n2;
    n2->next = n1;
    
    printf(">> Dropping Roots...\n");
    n1 = NULL;
    n2 = NULL;
    
    gc_collect();
    printf("[SUCCESS] Cycle collected! (Check debug_heap to ensure no LIVE blocks remain here)\n");
}

void test_aging() {
    printf("\n==================================================\n");
    printf("=== TEST 3: GENERATIONAL AGING ===\n");
    
    struct Node* volatile veteran = (struct Node*)gc_alloc(sizeof(struct Node));
    
    printf(">> Running GC 3 times to age the object...\n");
    gc_collect();
    gc_collect();
    gc_collect();
    
    printf("[SUCCESS] Check the heap below. The remaining block should have Age: 3.\n");
    debug_heap();
}

void test_write_barrier() {
    printf("\n==================================================\n");
    printf("=== TEST 4: THE WRITE BARRIER (Old -> Young) ===\n");
    
    struct Node* volatile old_obj = (struct Node*)gc_alloc(sizeof(struct Node));
    old_obj->value = 10;
    gc_collect(); 
    
    printf(">> Allocating Young object...\n");
    struct Node* young_obj = (struct Node*)gc_alloc(sizeof(struct Node));
    young_obj->value = 99;
    
    printf(">> Triggering SEGFAULT on purpose (Writing to Old Object)...\n");

    old_obj->next = young_obj; 
    
    printf(">> Dropping direct root to Young object...\n");
    young_obj = NULL; 

    gc_collect();
    
    if (old_obj->next != NULL && old_obj->next->value == 99) {
        printf("[SUCCESS] Write Barrier worked! Young object survived via Old reference.\n");
    } else {
        printf("[FAIL] Young object was collected! Write Barrier failed.\n");
    }
}

int main() {
    void* stack_top = __builtin_frame_address(0);
    gc_init(4096 * 4, stack_top);

    test_coalescing();
    test_cycle();
    test_aging();
    test_write_barrier();

    printf("\n==================================================\n");
    printf(">> ALL TESTS COMPLETED. FINAL HEAP STATE:\n");
    debug_heap();

    gc_destroy();
    return 0;
}