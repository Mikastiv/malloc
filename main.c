#include "memory.h"

int main() {
    void* block = malloc(43902);
    void* block1 = malloc(64);
    malloc(64);
    malloc(126);
    malloc(32);
    malloc(1026);
    show_alloc_mem();
    free(block);
    free(block1);
    block = malloc(42);
    show_alloc_mem();
}
