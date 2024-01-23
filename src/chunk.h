#pragma once

#include "types.h"

#include <stdbool.h>

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
chunk_unmapped_size(const u64 requested_size);

u64
chunk_mapped_size(const u64 requested_size);

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
