#include "memory.h"

void*
malloc(size_t size) {
    (void)size;
    return 0;
}

void*
realloc(void* ptr, size_t size) {
    (void)ptr;
    (void)size;
    return 0;
}

void
free(void* ptr) {
    (void)ptr;
}

void
show_alloc_mem(void) {
}
