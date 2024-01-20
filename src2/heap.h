#pragma once

#include "freelist.h"

#include <stdbool.h>

typedef struct Heap {
    Freelist freelist;
    struct Heap* next;
} Heap;

u64
heap_size(void);

u64
heap_metadata_size(void);

Heap*
heap_from_ptr(void* ptr);

bool
ptr_in_heap(Heap* heap, void* ptr);

Chunk*
heap_to_chunk(Heap* heap);
