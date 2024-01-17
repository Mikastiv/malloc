#include "memory.h"

int main() {
    void* block = malloc(64);
    void* block1 = malloc(64);
    void* block65 = malloc(126);
    void* block2 = malloc(32);
    void* block3 = malloc(1026);
    show_alloc_mem();
}
