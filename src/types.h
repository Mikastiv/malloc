#pragma once

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef enum ChunkFlag {
    ChunkFlag_Allocated = 1 << 0,
    ChunkFlag_Mapped = 1 << 1,
    ChunkFlag_First = 1 << 2,
    ChunkFlag_Last = 1 << 3,
} ChunkFlag;

typedef struct Chunk {
    u64 prev_size;
    u64 user_size;
    u64 flags : 4;
    u64 size  : 60;
    struct Chunk* next; // only use if free
    struct Chunk* prev; // only use if free
} Chunk;

typedef struct MappedChunk {
    struct MappedChunk* next;
} MappedChunk;

typedef struct MappedChunkList {
    MappedChunk* head;
} MappedChunkList;

typedef struct Freelist {
    Chunk* head;
} Freelist;

typedef struct Heap {
    Freelist freelist;
    u64 size;
    struct Heap* next;
} Heap;

typedef enum ArenaType {
    ArenaType_Tiny = 0,
    ArenaType_Small,
} ArenaType;

typedef struct Arena {
    u64 len;
    ArenaType type;
    Heap* head;
} Arena;
