#include "types.h"
#include "utils.h"

#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

#define CHUNK_ALIGNMENT 16
#define MAX_TINY_SIZE 128
#define MIN_LARGE_SIZE 4096
#define MIN_BLOCK_SIZE CHUNK_ALIGNMENT
#define MIN_CHUNK_SIZE (CHUNK_ALIGNMENT * 2 + MIN_BLOCK_SIZE)

typedef struct ChunkHeader {
    u64 flags : 2;
    u64 size  : 62;
    u64 user_size;
} ChunkHeader;

typedef enum ChunkFlag {
    ChunkFlag_Allocated = 1 << 0,
    ChunkFlag_Mapped = 1 << 1,
} ChunkFlag;

typedef struct FreeChunk {
    ChunkHeader header;
    struct FreeChunk* prev;
    struct FreeChunk* next;
} FreeChunk;

typedef struct Freelist {
    FreeChunk* head;
} Freelist;

typedef struct Heap {
    u64 size;
    struct Heap* next;
} Heap;

typedef struct Arena {
    Heap* head;
} Arena;

typedef struct Context {
    bool is_init;
    u64 page_size;

    Arena arena_tiny;
    Arena arena_small;
    Freelist chunks_tiny;
    Freelist chunks_small;
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

    ctx.page_size = (u64)getpagesize();
    ctx.is_init = true;

    ctx.arena_tiny.head = NULL;
    ctx.arena_small.head = NULL;
    ctx.chunks_tiny.head = NULL;
    ctx.chunks_small.head = NULL;

    return true;
}

static __attribute__((destructor)) void
deinit(void) {
    if (!ctx.is_init) return;
    pthread_mutex_destroy(&mtx);
}

static u64
header_size(void) {
    return align_up(sizeof(ChunkHeader), CHUNK_ALIGNMENT);
}

static u64
chunk_metadata_size(const bool is_mapped) {
    return is_mapped ? header_size() : header_size() * 2; // header + footer
}

static char*
chunk_data_start(ChunkHeader* header) {
    return (char*)header + header_size();
}

static ChunkHeader*
get_header(void* ptr) {
    char* block = ptr;
    return (ChunkHeader*)(block - header_size());
}

static ChunkHeader*
get_footer(ChunkHeader* header) {
    char* ptr = (char*)header;
    ptr += header->size - header_size();
    return (ChunkHeader*)ptr;
}

static bool
mmap_failed(void* ptr) {
    return (u64)ptr == (u64)-1;
}

static u64
calculate_chunk_size(const u64 requested_size, const bool is_mapped) {
    const u64 user_block_size = align_up(requested_size, CHUNK_ALIGNMENT);
    const u64 chunk_size = align_up(user_block_size + chunk_metadata_size(is_mapped), CHUNK_ALIGNMENT);
    return chunk_size;
}

static char*
get_block_from_tiny_list(const u64 requested_size) {
    const u64 size = calculate_chunk_size(requested_size, false);
    (void)size;

    return NULL;
}

static u64
heap_metadata_size(void) {
    return align_up(sizeof(Heap), CHUNK_ALIGNMENT);
}

static ChunkHeader*
heap_data_start(Heap* heap) {
    char* ptr = (char*)heap + heap_metadata_size();
    return (ChunkHeader*)ptr;
}

static void
append_heap(Arena* arena, Heap* heap, const u64 size) {
    if (arena->head == NULL) {
        arena->head = heap;
        arena->head->size = size;
        arena->head->next = NULL;
    } else {
        Heap* old_head = arena->head;
        arena->head = heap;
        arena->head->size = size;
        arena->head->next = old_head;
    }
}

static bool
grow_arena(Arena* arena) {
    const u64 size = ctx.page_size * 8;
    void* heap = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mmap_failed(heap)) return false;

    append_heap(arena, heap, size);

    const u64 chunk_size = size - heap_metadata_size();
    ChunkHeader* header = heap_data_start(heap);
    *header = (ChunkHeader){ .size = chunk_size };

    ChunkHeader* footer = get_footer(header);
    *footer = (ChunkHeader){ .size = 0 }; // indicate last chunk footer

    return true;
}

static u64
user_block_size(const u64 chunk_size) {
    return chunk_size - chunk_metadata_size(false);
}

static ChunkHeader*
next_chunk_header(ChunkHeader* header) {
    char* ptr = (char*)header;
    return (ChunkHeader*)(ptr + header->size);
}

static ChunkHeader*
next_appropriate_chunk(ChunkHeader* current, const u64 size) {
    ChunkHeader* footer = get_footer(current);
    while (user_block_size(current->size) < size && footer->size != 0) {
        current = next_chunk_header(current);
        footer = get_footer(current);
    }
    return current;
}

static ChunkHeader*
split_chunk(ChunkHeader* header) {
}

static char*
get_block_from_heap(Arena arena, const u64 requested_size) {
    const u64 size = calculate_chunk_size(requested_size, false);
    Heap* heap = arena.head;
    char* block = NULL;

    while (heap) {
        ChunkHeader* header = next_appropriate_chunk(heap_data_start(heap), size);
        ChunkHeader* footer = get_footer(header);
        while (header->flags & ChunkFlag_Allocated && footer->size != 0) {
            header = next_appropriate_chunk(header, size);
            footer = get_footer(header);
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
        const u64 chunk_size = calculate_chunk_size(size, true);
        char* chunk = mmap(NULL, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(chunk)) goto error;

        ChunkHeader* header = (ChunkHeader*)chunk;
        *header = (ChunkHeader){ .size = chunk_size, .flags = ChunkFlag_Mapped, .user_size = size };

        unlock_mutex();
        return chunk_data_start(header);
    }

    if (size <= MAX_TINY_SIZE) {
        // get from freelist
        char* block = get_block_from_tiny_list(size);
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
        if (!grow_arena(&ctx.arena_tiny)) {
            unlock_mutex();
            return NULL;
        }

        return get_block_from_heap(ctx.arena_tiny, size);
    }

    return NULL;
error:
    unlock_mutex();
    return NULL;
}

void*
realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);

    lock_mutex();

    ChunkHeader* header = get_header(ptr);

    if (header->flags & ChunkFlag_Mapped) {
        if (size <= header->size - chunk_metadata_size(true)) {
            header->user_size = size;
            unlock_mutex();
            return ptr;
        };

        const u64 new_chunk_size = calculate_chunk_size(size, true);
        char* new_chunk = mmap(NULL, new_chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(new_chunk)) goto error;

        memcopy(new_chunk + header_size(), ptr, header->user_size);
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

    ChunkHeader* header = get_header(ptr);

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
