#include "freelist.h"

#include "defines.h"

void
freelist_prepend(Freelist* list, ChunkHeader* chunk) {
    FreeChunk* ptr = (FreeChunk*)chunk;
    ptr->header = *chunk;
    if (list->head) {
        FreeChunk* tmp = list->head;
        list->head = ptr;
        ptr->prev = 0;
        ptr->next = tmp;
        tmp->prev = ptr;
    } else {
        list->head = ptr;
        ptr->next = 0;
        ptr->prev = 0;
    }
}

char*
freelist_get_block(Freelist* list, const u64 requested_size) {
    const u64 size = chunk_calculate_size(requested_size, false);
    char* block = 0;

    FreeChunk* ptr = list->head;
    while (ptr) {
        if (ptr->header.size >= size) break;
        ptr = ptr->next;
    }

    if (ptr) {
        if (ptr == list->head) {
            list->head = ptr->next;
            if (list->head) list->head->prev = 0;
        } else {
            ptr->prev->next = ptr->next;
            if (ptr->next) ptr->next->prev = ptr->prev;
        }

        if (ptr->header.size - size > MIN_CHUNK_SIZE) {
            ChunkHeader* other = chunk_split(&ptr->header, size);
            freelist_prepend(list, other);
        }

        ChunkHeader* footer = chunk_get_footer(&ptr->header);
        const bool is_last = footer->size == 0;
        ptr->header.flags |= ChunkFlag_Allocated;
        ptr->header.user_size = requested_size;
        *footer = ptr->header;
        if (is_last) footer->size = 0;
        block = chunk_data_start(&ptr->header);
    }

    return block;
}
