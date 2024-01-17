#pragma once

#define CHUNK_ALIGNMENT 16 // Must be >= sizeof(ChunkHeader)
#define MIN_BLOCK_SIZE CHUNK_ALIGNMENT // Must be >= sizeof(FreeChunk)
#define METADATA_SIZE (CHUNK_ALIGNMENT * 2)
#define MIN_CHUNK_SIZE (METADATA_SIZE + MIN_BLOCK_SIZE)
#define MAX_TINY_SIZE 128
#define MIN_LARGE_SIZE (4096 - CHUNK_ALIGNMENT)
