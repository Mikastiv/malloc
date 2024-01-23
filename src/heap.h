#pragma once

#include "types.h"

#include <stdbool.h>

u64
heap_size(const ArenaType type);

u64
heap_metadata_size(void);

bool
ptr_in_heap(Heap* heap, void* ptr);

Chunk*
heap_to_chunk(Heap* heap);

Chunk*
heap_find_chunk(Heap* heap, const u64 size);
