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
