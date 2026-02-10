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
    void* stack_top = (void*)&stack_top;
    gc_init(4096, stack_top);

    printf("=== SERVER SIMULATION ===\n");

    printf(">> Creating User Session (Persistent Object)...\n");
    volatile DataObject* session = (DataObject*)gc_alloc(sizeof(DataObject));
    session->id = 1;
    session->value = 99.9;
    printf("   Session allocated at: %p\n", (void*)session);

    debug_heap();

    printf("\n>> Processing requests...\n");
    create_temporary_garbage();
    
    
    printf("\n>> GC woke up! Cleaning the house...\n");
    gc_collect();

    printf("\n>> Final Memory State:\n");
    debug_heap();

    printf(
        "\nValidation: Is session ID %d still alive?\n",
        session->id
    );

    gc_destroy();

    return 0;
}
