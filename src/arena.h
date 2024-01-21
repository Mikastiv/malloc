#pragma once

#include "heap.h"

#include <stdbool.h>

typedef struct Arena {
    Heap* head;
} Arena;

bool
arena_grow(Arena* arena);

Heap*
arena_find_heap(Arena* arena, void* ptr);

Chunk*
arena_find_chunk(Arena* arena, const u64 size);
