#include "heap.h"

#include "defines.h"
#include "utils.h"

u64
heap_metadata_size(void) {
    return align_up(sizeof(Heap), CHUNK_ALIGNMENT);
}

ChunkHeader*
heap_data_start(Heap* heap) {
    char* ptr = (char*)heap + heap_metadata_size();
    return (ChunkHeader*)ptr;
}

char*
heap_get_block(Heap* heap, const u64 requested_size) {
    const u64 size = chunk_calculate_size(requested_size, false);
    char* block = 0;

    while (heap) {
        ChunkHeader* header = heap_data_start(heap);
        ChunkHeader* footer = chunk_get_footer(header);
        while (footer->size != 0) {
            if ((header->flags & ChunkFlag_Allocated) == 0 && header->size >= size) break;
            header = chunk_next(header);
            footer = chunk_get_footer(header);
        }

        if (header->size >= size) {
            if (header->size - size > MIN_CHUNK_SIZE) {
                chunk_split(header, size);
            }
            footer = chunk_get_footer(header);
            header->flags |= ChunkFlag_Allocated;
            header->user_size = requested_size;
            *footer = *header;
            block = chunk_data_start(header);
            break;
        }

        heap = heap->next;
    }

    return block;
}
