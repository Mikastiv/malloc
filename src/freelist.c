#include "freelist.h"

void
freelist_prepend(Freelist* list, Chunk* chunk) {
    if (list->head == chunk) return;

    if (list->head) {
        Chunk* ptr = list->head;
        list->head = chunk;
        chunk->next = ptr;
        chunk->prev = 0;
        ptr->prev = chunk;
    } else {
        list->head = chunk;
        chunk->next = 0;
        chunk->prev = 0;
    }
}
