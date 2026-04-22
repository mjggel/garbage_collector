#include <stdio.h>
#include "arena.h"

typedef struct {
    int id;
    double value;
} DataObject;

void create_temporary_garbage() {
    printf("  [Job] Generating temporary garbage...\n");
    for (int i = 0; i < 3; i++) {
        DataObject* temp = (DataObject*)gc_alloc(sizeof(DataObject));
        temp->id = 100 + i;
    }
}

int main() {
    void* stack_top = __builtin_frame_address(0);
    gc_init(4096, stack_top);

    printf("=== SERVER SIMULATION (GenGC Edition) ===\n");

    printf(">> Creating User Session (Persistent Object)...\n");
    DataObject* volatile session = (DataObject*)gc_alloc(sizeof(DataObject));
    session->id = 1;
    session->value = 99.9;
    printf("   Session allocated at: %p\n", (void*)session);

    debug_heap();

    printf("\n>> Processing requests...\n");
    create_temporary_garbage();
    
    printf("\n>> GC woke up! Cleaning up and locking the Arena...\n");
    gc_collect();

    printf("\n>> Heap state after first GC:\n");
    debug_heap();

    printf("\n>> [TEST] Attempting to modify protected Session (Old Generation)...\n");
    
    session->value = 500.5; 

    printf(">> [SUCCESS] Write Barrier intercepted and allowed the write!\n");
    printf("   New Session value: %.1f\n", session->value);

    printf("\n>> Running final GC (should scan dirty pages)...\n");
    gc_collect();

    printf("\n>> Final check: Is Session ID %d still alive?\n", session->id);
    
    gc_destroy();

    return 0;
}