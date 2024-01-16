#pragma once

#include "chunk.h"
#include "types.h"

typedef struct Heap {
    u64 size;
    struct Heap* next;
} Heap;

u64
heap_metadata_size(void);

ChunkHeader*
heap_data_start(Heap* heap);

u64
heap_metadata_size(void);

ChunkHeader*
heap_data_start(Heap* heap);
