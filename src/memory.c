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

typedef struct ArenaNode {
    u64 size;
    struct ArenaNode* next;
} ArenaNode;

typedef struct Arena {
    ArenaNode* head;
} Arena;

typedef struct Context {
    bool is_init;
    u64 page_size;
    u64 header_size;

    Arena arena;
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
init(Context* ctx) {
    if (ctx->is_init) return true;

    if (pthread_mutex_init(&mtx, NULL) != 0) return false;

    ctx->page_size = (u64)getpagesize();
    ctx->header_size = align_up(sizeof(ChunkHeader), CHUNK_ALIGNMENT);
    ctx->is_init = true;

    return true;
}

static __attribute__((destructor)) void
deinit(void) {
    if (!ctx.is_init) return;
    pthread_mutex_destroy(&mtx);
}

static u64
metadata_size(const bool is_mapped) {
    return is_mapped ? ctx.header_size : ctx.header_size * 2; // header + footer
}

static ChunkHeader*
get_header(void* ptr) {
    char* block = ptr;
    return (ChunkHeader*)(block - ctx.header_size);
}

static bool
mmap_failed(void* ptr) {
    return (u64)ptr == (u64)-1;
}

static u64
calculate_chunk_size(const u64 requested, const bool is_mapped) {
    const u64 user_block_size = align_up(requested, CHUNK_ALIGNMENT);
    const u64 chunk_size = align_up(user_block_size + metadata_size(is_mapped), CHUNK_ALIGNMENT);
    return chunk_size;
}

void*
malloc(size_t size) {
    lock_mutex();

    if (!ctx.is_init) {
        if (!init(&ctx)) goto error;
    }

    if (size >= MIN_LARGE_SIZE) {
        const u64 chunk_size = calculate_chunk_size(size, true);
        char* chunk = mmap(NULL, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(chunk)) goto error;

        ChunkHeader* header = (ChunkHeader*)chunk;
        *header = (ChunkHeader){ .size = chunk_size, .flags = ChunkFlag_Mapped, .user_size = size };

        unlock_mutex();
        return chunk + ctx.header_size;
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
        if (size <= header->size - metadata_size(true)) {
            header->user_size = size;
            unlock_mutex();
            return ptr;
        };

        const u64 new_chunk_size = calculate_chunk_size(size, true);
        char* new_chunk = mmap(NULL, new_chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(new_chunk)) goto error;

        memcopy(new_chunk + ctx.header_size, ptr, header->user_size);
        ChunkHeader* new_header = (ChunkHeader*)new_chunk;
        *new_header = (ChunkHeader){ .size = new_chunk_size, .flags = ChunkFlag_Mapped, .user_size = size };

        munmap(header, header->size);
        unlock_mutex();
        return new_chunk + ctx.header_size;
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
