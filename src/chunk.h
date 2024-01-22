#pragma once

#include "types.h"

#include <stdbool.h>

typedef enum ChunkFlag {
    ChunkFlag_Allocated = 1 << 0,
    ChunkFlag_Mapped = 1 << 1,
    ChunkFlag_First = 1 << 2,
    ChunkFlag_Last = 1 << 3,
} ChunkFlag;

typedef struct Chunk {
    u64 prev_size;
    u64 flags : 4;
    u64 size  : 60;
    u64 user_size;
    struct Chunk* next; // only use if free
    struct Chunk* prev; // only use if free
} Chunk;

typedef struct MappedChunk {
    struct MappedChunk* next;
} MappedChunk;

typedef struct MappedChunkList {
    MappedChunk* head;
} MappedChunkList;

u64
chunk_alignment(void);

u64
chunk_min_size(void);

u64
chunk_max_tiny_size(void);

u64
chunk_min_large_size(void);

u64
chunk_metadata_size(const bool is_mapped);

u64
mapped_chunk_metadata_size(void);

void
chunk_set_footer(Chunk* chunk);

Chunk*
chunk_from_mem(void* mem);

void*
chunk_to_mem(Chunk* chunk);

Chunk*
chunk_from_mapped(MappedChunk* mapped);

MappedChunk*
chunk_to_mapped(Chunk* chunk);

Chunk*
chunk_next(Chunk* chunk);

Chunk*
chunk_prev(Chunk* chunk);

u64
chunk_calculate_size(const u64 requested_size, const bool is_mapped);

u64
chunk_usable_size(Chunk* chunk);

Chunk*
chunk_coalesce(Chunk* front, Chunk* back);

Chunk*
chunk_split(Chunk* chunk, const u64 size);

bool
chunk_is_mapped(Chunk* chunk);

bool
chunk_is_allocated(Chunk* chunk);
