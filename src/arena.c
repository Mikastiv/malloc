#include "arena.h"

#include "utils.h"

#include <sys/mman.h>
#include <unistd.h>

static void
append_heap(Arena* arena, Heap* heap, const u64 size) {
    if (arena->head == NULL) {
        arena->head = heap;
        arena->head->size = size;
        arena->head->next = NULL;
    } else {
        Heap* old_head = arena->head;
        arena->head = heap;
        arena->head->size = size;
        arena->head->next = old_head;
    }
}

bool
arena_grow(Arena* arena) {
    const u64 size = (u64)getpagesize() * 8;
    void* heap = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mmap_failed(heap)) return false;

    append_heap(arena, heap, size);

    const u64 chunk_size = size - heap_metadata_size();
    ChunkHeader* header = heap_data_start(heap);
    *header = (ChunkHeader){ .size = chunk_size };

    ChunkHeader* footer = chunk_get_footer(header);
    *footer = (ChunkHeader){ .size = 0 }; // indicate last chunk footer

    return true;
}
