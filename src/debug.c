#include "chunk.h"
#include "heap.h"

#include <assert.h>

void
check_all_mem(Arena* arena) {
    Heap* heap = arena->head;
    while (heap) {
        Chunk* chunk = heap_to_chunk(heap);
        while (chunk) {
            Chunk* next = chunk_next(chunk);
            if (next && next->prev_size != chunk->size) {
                volatile char* ptr = 0;
                *ptr = 1;
            }
            chunk = next;
        }
        heap = heap->next;
    }
}
