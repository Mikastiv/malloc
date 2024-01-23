#include "arena.h"

#include "chunk.h"
#include "heap.h"
#include "utils.h"

#include <sys/mman.h>

static void
prepend_heap(Arena* arena, Heap* heap) {
    if (arena->head) {
        Heap* old_head = arena->head;
        arena->head = heap;
        arena->head->next = old_head;
    } else {
        arena->head = heap;
        arena->head->next = 0;
    }
    arena->len++;
}

bool
arena_grow(Arena* arena) {
    const u64 size = heap_size(arena->type);
    Heap* heap = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mmap_failed(heap)) return false;

    prepend_heap(arena, heap);

    Chunk* chunk = heap_to_chunk(heap);
    chunk->size = size - heap_metadata_size();
    chunk->flags = ChunkFlag_First | ChunkFlag_Last;

    heap->size = size;
    heap->freelist.head = chunk;
    chunk->next = 0;
    chunk->prev = 0;

    return true;
}

Heap*
arena_find_heap(Arena* arena, void* ptr) {
    Heap* heap = arena->head;
    while (heap) {
        if (ptr_in_heap(heap, ptr)) return heap;
        heap = heap->next;
    }

    return 0;
}

Chunk*
arena_find_chunk(Arena* arena, const u64 size) {
    Chunk* chunk = 0;
    Heap* heap = arena->head;
    while (heap) {
        chunk = heap_find_chunk(heap, size);
        if (chunk) break;
        heap = heap->next;
    }
    return chunk;
}

void
arena_remove_heap(Arena* arena, Heap* heap) {
    Heap* ptr = arena->head;
    arena->len--;
    if (ptr == heap) {
        arena->head = ptr->next;
    } else {
        Heap* tmp = ptr;
        ptr = ptr->next;
        while (ptr) {
            if (ptr == heap) {
                tmp->next = heap->next;
                return;
            }
            tmp = ptr;
            ptr = ptr->next;
        }
    }
}

ArenaType
arena_select(const u64 requested_size) {
    const u64 size = chunk_calculate_size(requested_size, false);
    if (size <= chunk_max_tiny_size())
        return ArenaType_Tiny;
    else
        return ArenaType_Small;
}
