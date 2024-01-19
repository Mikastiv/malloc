#pragma once

#include "chunk.h"

typedef struct FreeChunk {
    ChunkHeader header;
    struct FreeChunk* prev;
    struct FreeChunk* next;
} FreeChunk;

typedef struct Freelist {
    u64 len;
    FreeChunk* head;
} Freelist;

void
freelist_prepend(Freelist* list, ChunkHeader* chunk);

char*
freelist_get_block(Freelist* list, const u64 requested_size);
