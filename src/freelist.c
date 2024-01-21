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

void
freelist_remove(Freelist* list, Chunk* chunk) {
    if (list->head == chunk) {
        list->head = chunk->next;
        if (list->head) list->head->prev = 0;
    } else {
        Chunk* ptr = list->head;
        while (ptr) {
            if (ptr == chunk) {
                ptr->prev->next = ptr->next;
                ptr->next->prev = ptr->prev;
                return;
            }
            ptr = ptr->next;
        }
    }
}
