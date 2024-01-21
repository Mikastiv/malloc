#pragma once

#include "heap.h"

#include <stdbool.h>

typedef struct Arena {
    u64 len;
    Heap* head;
} Arena;

bool
arena_grow(Arena* arena);

Heap*
arena_find_heap(Arena* arena, void* ptr);

Chunk*
arena_find_chunk(Arena* arena, const u64 size);

void
arena_remove_heap(Arena* arena, Heap* heap);
