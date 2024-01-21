#pragma once

#include "heap.h"

#include <stdbool.h>

typedef struct Arena {
    Heap* head;
} Arena;

bool
arena_grow(Arena* arena);
