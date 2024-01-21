#pragma once

#include "chunk.h"
#include "freelist.h"
#include "types.h"

typedef struct Heap {
    Freelist* freelist;
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

char*
heap_get_block(Heap* heap, const u64 requested_size);