#pragma once

#include "types.h"

void
freelist_prepend(Freelist* list, Chunk* chunk);

void
freelist_remove(Freelist* list, Chunk* chunk);
