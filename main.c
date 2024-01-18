#include "memory.h"

int main() {
    malloc(64);
    malloc(64);
    malloc(126);
    malloc(32);
    malloc(1026);
    malloc(43902);
    show_alloc_mem();
}
