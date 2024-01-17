#pragma once

#define CHUNK_ALIGNMENT 16 // Must be >= sizeof(ChunkHeader)
#define MAX_TINY_SIZE 128
#define MIN_LARGE_SIZE 4096
#define MIN_BLOCK_SIZE CHUNK_ALIGNMENT // Must be >= sizeof(FreeChunk)
#define MIN_CHUNK_SIZE (CHUNK_ALIGNMENT * 2 + MIN_BLOCK_SIZE)
