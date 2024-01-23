#pragma once

#include "types.h"

#include <stdbool.h>

bool
arena_grow(Arena* arena);

Heap*
arena_find_heap(Arena* arena, void* ptr);

Chunk*
arena_find_chunk(Arena* arena, const u64 size);

void
arena_remove_heap(Arena* arena, Heap* heap);

ArenaType
arena_select(const u64 requested_size);
