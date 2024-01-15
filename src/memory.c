#include "types.h"
#include "utils.h"

#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

#define CHUNK_ALIGNMENT 16
#define MAX_TINY_SIZE 128
#define MIN_LARGE_SIZE 4096

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

    Arena arena;
    Freelist tiny_chunks;
    Freelist small_chunks;
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

    ctx.arena.head = NULL;
    ctx.tiny_chunks.head = NULL;
    ctx.small_chunks.head = NULL;

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
get_block_from_tiny(const u64 requested_size) {
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


static bool
grow_arena(void) {
    const u64 size = ctx.page_size * 8;
    void* heap = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (mmap_failed(heap)) return false;

    if (ctx.arena.head == NULL) {
        ctx.arena.head = heap;
        ctx.arena.head->size = size;
        ctx.arena.head->next = NULL;
    } else {
        Heap* old_head = ctx.arena.head;
        ctx.arena.head = heap;
        ctx.arena.head->size = size;
        ctx.arena.head->next = old_head;
    }

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

static char*
get_block_from_heap(const u64 requested_size) {
    Heap* heap = ctx.arena.head;
    char* block = NULL;

    while (heap) {
        ChunkHeader* header = heap_data_start(heap);
        ChunkHeader* footer = get_footer(header);
        while (user_block_size(header->size) < requested_size && footer->size != 0) {
            header = next_chunk_header(header);
            footer = get_footer(header);
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
        return chunk + header_size();
    }

    if (size <= MAX_TINY_SIZE) {
        char* block = get_block_from_tiny(size);
        if (block) {
            unlock_mutex();
            return block;
        }

        grow_arena();

        return NULL;
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
        return new_chunk + header_size();
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
