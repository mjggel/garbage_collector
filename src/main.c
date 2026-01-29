#include <stdio.h>
#include "arena.h"

int main() {

    gc_init(4096);
    void* p1 = gc_alloc(64);
    void* p2 = gc_alloc(128);


    printf("--- Day 0: Sanity Check ---\n");
    printf("Size of struct Block: %zu bytes\n", sizeof(Block));
    
    if (sizeof(Block) % 8 == 0) {
        printf("Check: Struct aligned to 8 bytes. Perfect for pointer tagging.\n");
    } else {
        printf("Warning: Struct not aligned. We will have problems with the bits.\n");
    }

    uintptr_t fake_ptr = 0x123456780;
    uintptr_t tagged_ptr = fake_ptr | TAG_FREE | TAG_MARK;
    
    printf("Original Ptr: %p\n", (void*)fake_ptr);
    printf("Tagged Ptr:   %p\n", (void*)tagged_ptr);
    printf("Untagged Ptr: %p\n", (void*)UNTAG_BLOCK(tagged_ptr));

    return 0;
}
