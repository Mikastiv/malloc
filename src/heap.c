#include "heap.h"

#include "chunk.h"
#include "utils.h"

#include <unistd.h>

u64
heap_size(const ArenaType type) {
    u64 size;

    if (type == ArenaType_Tiny) {
        size = align_up(100 * chunk_max_tiny_size() + heap_metadata_size(), getpagesize());
    } else {
        size = align_up(100 * chunk_min_large_size() + heap_metadata_size(), getpagesize());
    }

    return size;
}

u64
heap_metadata_size(void) {
    return align_up(sizeof(Heap), chunk_alignment());
}

bool
ptr_in_heap(Heap* heap, void* ptr) {
    return (u64)heap < (u64)ptr && (u64)heap + heap->size >= (u64)ptr;
}

Chunk*
heap_to_chunk(Heap* heap) {
    return (Chunk*)((char*)heap + heap_metadata_size());
}

Chunk*
heap_find_chunk(Heap* heap, const u64 size) {
    Chunk* ptr = heap->freelist.head;
    while (ptr) {
        if (ptr->size >= size) return ptr;
        ptr = ptr->next;
    }
    return 0;
}
