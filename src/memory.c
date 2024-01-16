#include "arena.h"
#include "chunk.h"
#include "defines.h"
#include "types.h"
#include "utils.h"

#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct Context {
    bool is_init;

    Arena arena_tiny;
    Arena arena_small;
    Freelist freelist_tiny;
    Freelist freelist_small;
} Context;

static Context ctx;
static pthread_mutex_t mtx;

static int
lock_mutex(void) {
    return pthread_mutex_lock(&mtx);
}

static int
unlock_mutex(void) {
    return pthread_mutex_unlock(&mtx);
}

static bool
init(void) {
    if (ctx.is_init) return true;

    if (pthread_mutex_init(&mtx, NULL) != 0) return false;

    ctx.is_init = true;

    ctx.arena_tiny.head = NULL;
    ctx.arena_small.head = NULL;
    ctx.freelist_tiny.head = NULL;
    ctx.freelist_small.head = NULL;

    return true;
}

static __attribute__((destructor)) void
deinit(void) {
    if (!ctx.is_init) return;
    pthread_mutex_destroy(&mtx);
}

static char*
get_block_from_freelist(Freelist list, const u64 requested_size) {
    const u64 size = chunk_calculate_size(requested_size, false);
    (void)size;

    return NULL;
}

static u64
user_block_size(const u64 chunk_size) {
    return chunk_size - chunk_metadata_size(false);
}

static ChunkHeader*
next_appropriate_chunk(ChunkHeader* current, const u64 size) {
    ChunkHeader* footer = chunk_get_footer(current);
    while (user_block_size(current->size) < size && footer->size != 0) {
        current = chunk_next_header(current);
        footer = chunk_get_footer(current);
    }
    return current;
}

static ChunkHeader*
split_chunk(ChunkHeader* header) {
}

static char*
get_block_from_heap(Arena arena, const u64 requested_size) {
    const u64 size = chunk_calculate_size(requested_size, false);
    Heap* heap = arena.head;
    char* block = NULL;

    while (heap) {
        ChunkHeader* header = next_appropriate_chunk(heap_data_start(heap), size);
        ChunkHeader* footer = chunk_get_footer(header);
        while (header->flags & ChunkFlag_Allocated && footer->size != 0) {
            header = next_appropriate_chunk(header, size);
            footer = chunk_get_footer(header);
        }

        if (header->size <= size) {
            if (header->size - size > MIN_CHUNK_SIZE) {
                ChunkHeader* other_half = split_chunk(header);
            }
            block = chunk_data_start(header);
        }
    }

    return block;
}

void*
malloc(size_t size) {
    lock_mutex();

    if (!ctx.is_init) {
        if (!init()) goto error;
    }

    if (size >= MIN_LARGE_SIZE) {
        const u64 chunk_size = chunk_calculate_size(size, true);
        char* chunk = mmap(NULL, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(chunk)) goto error;

        ChunkHeader* header = (ChunkHeader*)chunk;
        *header = (ChunkHeader){ .size = chunk_size, .flags = ChunkFlag_Mapped, .user_size = size };

        unlock_mutex();
        return chunk_data_start(header);
    }

    if (size <= MAX_TINY_SIZE) {
        // get from freelist
        char* block = get_block_from_freelist(ctx.freelist_tiny, size);
        if (block) {
            unlock_mutex();
            return block;
        }

        // get from top of heap
        block = get_block_from_heap(ctx.arena_tiny, size);
        if (block) {
            unlock_mutex();
            return block;
        }

        // grow heap and get block
        if (!arena_grow(&ctx.arena_tiny)) {
            unlock_mutex();
            return NULL;
        }

        return get_block_from_heap(ctx.arena_tiny, size);
    }

    // get from freelist
    char* block = get_block_from_freelist(ctx.freelist_small, size);
    if (block) {
        unlock_mutex();
        return block;
    }

    // get from top of heap
    block = get_block_from_heap(ctx.arena_small, size);
    if (block) {
        unlock_mutex();
        return block;
    }

    // grow heap and get block
    if (!arena_grow(&ctx.arena_small)) {
        unlock_mutex();
        return NULL;
    }

    return get_block_from_heap(ctx.arena_small, size);
error:
    unlock_mutex();
    return NULL;
}

void*
realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);

    lock_mutex();

    ChunkHeader* header = chunk_get_header(ptr);

    if (header->flags & ChunkFlag_Mapped) {
        if (size <= header->size - chunk_metadata_size(true)) {
            header->user_size = size;
            unlock_mutex();
            return ptr;
        };

        const u64 new_chunk_size = chunk_calculate_size(size, true);
        char* new_chunk = mmap(NULL, new_chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(new_chunk)) goto error;

        memcopy(new_chunk + chunk_header_size(), ptr, header->user_size);
        ChunkHeader* new_header = (ChunkHeader*)new_chunk;
        *new_header = (ChunkHeader){ .size = new_chunk_size, .flags = ChunkFlag_Mapped, .user_size = size };

        munmap(header, header->size);
        unlock_mutex();
        return chunk_data_start(new_header);
    }

    return NULL;

error:
    unlock_mutex();
    return NULL;
}

void
free(void* ptr) {
    if (!ptr) return;

    lock_mutex();

    ChunkHeader* header = chunk_get_header(ptr);

    if (header->flags & ChunkFlag_Mapped) {
        munmap(header, header->size);
        unlock_mutex();
        return;
    }

    unlock_mutex();
}

void
show_alloc_mem(void) {
}
