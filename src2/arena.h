#pragma once

#include "heap.h"

#include <stdbool.h>

typedef struct Arena {
    Heap* head;
} Arena;

Heap*
arena_grow(Arena* arena);

Heap*
arena_find_heap(Arena* arena, void* ptr);
