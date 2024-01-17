#pragma once

#include "types.h"
#include <stdbool.h>

typedef struct ChunkHeader {
    u64 flags : 3;
    u64 size  : 61;
    u64 user_size;
} ChunkHeader;

typedef enum ChunkFlag {
    ChunkFlag_Allocated = 1 << 0,
    ChunkFlag_Mapped = 1 << 1,
    ChunkFlag_First = 1 << 2,
} ChunkFlag;

u64
chunk_header_size(void);

u64
chunk_metadata_size(const bool is_mapped);

char*
chunk_data_start(ChunkHeader* header);

ChunkHeader*
chunk_get_header(void* ptr);

ChunkHeader*
chunk_get_footer(ChunkHeader* header);

u64
chunk_calculate_size(const u64 requested_size, const bool is_mapped);

ChunkHeader*
chunk_split(ChunkHeader* header, const u64 size);

ChunkHeader*
chunk_next(ChunkHeader* header);

ChunkHeader*
chunk_prev(ChunkHeader* chunk);

ChunkHeader*
chunk_coalesce(ChunkHeader* front, ChunkHeader* back);
