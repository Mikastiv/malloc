#include "heap.h"

#include "utils.h"
#include "chunk.h"

#include <unistd.h>

u64
heap_size(void) {
    return getpagesize() * 8;
}

u64
heap_metadata_size(void) {
    return align_up(sizeof(Heap), chunk_alignment());
}


Heap*
heap_from_ptr(void* ptr) {
    return (Heap*)align_down((u64)ptr, heap_size());
}

bool
ptr_in_heap(Heap* heap, void* ptr) {
    return heap_from_ptr(ptr) == heap;
}

Chunk*
heap_to_chunk(Heap* heap) {
    return (Chunk*)((char*)heap + heap_metadata_size());
}
