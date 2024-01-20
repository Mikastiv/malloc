#pragma once

#include "chunk.h"

typedef struct Freelist {
    Chunk* head;
} Freelist;

void
freelist_prepend(Freelist* list, Chunk* chunk);
