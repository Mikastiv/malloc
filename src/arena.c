#include "arena.h"

#include "utils.h"

#include <sys/mman.h>
#include <unistd.h>

static void
append_heap(Arena* arena, Heap* heap) {
    if (arena->head == 0) {
        arena->head = heap;
        arena->head->next = 0;
    } else {
        Heap* old_head = arena->head;
        arena->head = heap;
        arena->head->next = old_head;
    }
}

bool
arena_grow(Arena* arena) {
    const u64 size = (u64)getpagesize() * 8;
    Heap* heap = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mmap_failed(heap)) return false;

    append_heap(arena, heap);

    const u64 chunk_size = size - heap_metadata_size();
    ChunkHeader* header = heap_data_start(heap);
    *header = (ChunkHeader){ .size = chunk_size, .flags = ChunkFlag_First };

    ChunkHeader* footer = chunk_get_footer(header);
    *footer = (ChunkHeader){ .size = 0, .flags = ChunkFlag_First }; // indicate last chunk footer

    return true;
}
