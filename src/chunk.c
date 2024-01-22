#include "chunk.h"

#include "utils.h"

#include <unistd.h>

u64
chunk_alignment(void) {
    return 16;
}

u64
chunk_min_size(void) {
    return align_up(sizeof(Chunk), chunk_alignment());
}

u64
chunk_max_tiny_size(void) {
    return align_up(256, chunk_alignment());
}

u64
chunk_min_large_size(void) {
    return align_up(4096, chunk_alignment());
}

u64
mapped_chunk_metadata_size(void) {
    return align_up(sizeof(MappedChunk), chunk_alignment());
}

u64
chunk_metadata_size(const bool is_mapped) {
    const u64 size = align_up(sizeof(u64) * 3, chunk_alignment());
    return is_mapped ? size + mapped_chunk_metadata_size() : size;
}

void
chunk_set_footer(Chunk* chunk) {
    Chunk* next = chunk_next(chunk);
    next->prev_size = chunk->size;
}

Chunk*
chunk_from_mem(void* mem) {
    return (Chunk*)((char*)mem - chunk_metadata_size(false));
}

void*
chunk_to_mem(Chunk* chunk) {
    return (char*)chunk + chunk_metadata_size(false);
}

Chunk*
chunk_from_mapped(MappedChunk* mapped) {
    return (Chunk*)((char*)mapped + mapped_chunk_metadata_size());
}

MappedChunk*
chunk_to_mapped(Chunk* chunk) {
    return (MappedChunk*)align_down((u64)chunk, getpagesize());
}

Chunk*
chunk_next(Chunk* chunk) {
    if (chunk->flags & ChunkFlag_Last) return 0;
    return (Chunk*)((char*)chunk + chunk->size);
}

Chunk*
chunk_prev(Chunk* chunk) {
    if (chunk->flags & ChunkFlag_First) return 0;
    return (Chunk*)((char*)chunk - chunk->prev_size);
}

u64
chunk_calculate_size(const u64 requested_size, const bool is_mapped) {
    const u64 user_block_size = align_up(requested_size, chunk_alignment());
    const u64 chunk_size = align_up(chunk_metadata_size(is_mapped), chunk_alignment());
    return user_block_size + chunk_size;
}

u64
chunk_usable_size(Chunk* chunk) {
    const u64 metadata_size = chunk_metadata_size(chunk_is_mapped(chunk));
    return chunk->size - metadata_size;
}

Chunk*
chunk_coalesce(Chunk* front, Chunk* back) {
    const bool back_is_last = back->flags & ChunkFlag_Last;

    front->size += back->size;
    if (back_is_last)
        front->flags |= ChunkFlag_Last;
    else
        chunk_set_footer(front);

    return front;
}

Chunk*
chunk_split(Chunk* chunk, const u64 size) {
    const bool is_last = chunk->flags & ChunkFlag_Last;

    const u64 old_size = chunk->size;
    chunk->size = size;
    chunk->flags &= ~ChunkFlag_Last;

    Chunk* next = chunk_next(chunk);
    next->prev_size = size;
    next->size = old_size - size;
    if (is_last) {
        next->flags = ChunkFlag_Last;
    } else {
        next->flags = 0;
        chunk_set_footer(next);
    }

    return next;
}

bool
chunk_is_mapped(Chunk* chunk) {
    return chunk->flags & ChunkFlag_Mapped;
}

bool
chunk_is_allocated(Chunk* chunk) {
    return chunk->flags & ChunkFlag_Allocated;
}
