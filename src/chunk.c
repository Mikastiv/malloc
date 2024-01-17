#include "chunk.h"

#include "defines.h"
#include "utils.h"

u64
chunk_header_size(void) {
    return align_up(sizeof(ChunkHeader), CHUNK_ALIGNMENT);
}

u64
chunk_metadata_size(const bool is_mapped) {
    return is_mapped ? chunk_header_size() : chunk_header_size() * 2; // header + footer
}

char*
chunk_data_start(ChunkHeader* header) {
    return (char*)header + chunk_header_size();
}

ChunkHeader*
chunk_get_header(void* ptr) {
    char* block = ptr;
    return (ChunkHeader*)(block - chunk_header_size());
}

ChunkHeader*
chunk_get_footer(ChunkHeader* header) {
    char* ptr = (char*)header;
    ptr += header->size - chunk_header_size();
    return (ChunkHeader*)ptr;
}

ChunkHeader*
chunk_next(ChunkHeader* header) {
    ChunkHeader* footer = chunk_get_footer(header);
    if (footer->size == 0) return 0;

    char* ptr = (char*)header + header->size;
    return (ChunkHeader*)ptr;
}

ChunkHeader*
chunk_prev(ChunkHeader* header) {
    if (header->flags & ChunkFlag_First) return 0;

    char* ptr = (char*)header - chunk_header_size();
    ChunkHeader* footer = (ChunkHeader*)ptr;

    header = (ChunkHeader*)(ptr - (footer->size - chunk_header_size()));
    return header;
}

u64
chunk_calculate_size(const u64 requested_size, const bool is_mapped) {
    const u64 user_block_size = align_up(requested_size, CHUNK_ALIGNMENT);
    const u64 chunk_size = align_up(user_block_size + chunk_metadata_size(is_mapped), CHUNK_ALIGNMENT);
    return chunk_size;
}

ChunkHeader*
chunk_split(ChunkHeader* header, const u64 size) {
    const u64 old_size = header->size;
    header->size = size;

    ChunkHeader* footer = chunk_get_footer(header);
    *footer = (ChunkHeader){ .size = size, .flags = header->flags };

    header = chunk_next(header);
    *header = (ChunkHeader){ .size = old_size - size, .flags = 0 };
    footer = chunk_get_footer(header);
    *footer = (ChunkHeader){ .size = old_size - size, .flags = 0 };

    return header;
}

ChunkHeader*
chunk_coalesce(ChunkHeader* front, ChunkHeader* back) {
    ChunkHeader* footer = chunk_get_footer(back);
    const bool back_is_last = footer->size == 0;

    front->size += back->size;
    footer = chunk_get_footer(front);
    *footer = *front;
    if (back_is_last) footer->size = 0;

    return front;
}
