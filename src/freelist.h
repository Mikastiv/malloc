#pragma once

#include "chunk.h"

typedef struct FreeChunk {
    ChunkHeader header;
    struct FreeChunk* prev;
    struct FreeChunk* next;
} FreeChunk;

typedef struct Freelist {
    FreeChunk* head;
} Freelist;

char*
freelist_get_block(Freelist* list, const u64 requested_size);
