#include "types.h"
#include "utils.h"

#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

#define CHUNK_ALIGNMENT 16
#define MAX_TINY_SIZE 128
#define MIN_LARGE_SIZE 4096

typedef struct {
    u64 flags : 2;
    u64 size  : 62;
    u64 user_size;
} ChunkHeader;

typedef enum {
    ChunkFlag_Allocated = 1 << 0,
    ChunkFlag_Mapped = 1 << 1,
} ChunkFlag;

// typedef struct {

// } Arena;

typedef struct {
    bool is_init;
    u64 page_size;
    u64 header_size;

    char* heap;
} Context;

static Context ctx;

static pthread_mutex_t mtx;

bool
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
metadata_size(void) {
    return ctx.header_size * 2;
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
calculate_chunk_size(const u64 requested) {
    const u64 user_block_size = align_up(requested, CHUNK_ALIGNMENT);
    const u64 chunk_size = align_up(user_block_size + metadata_size(), CHUNK_ALIGNMENT);
    return chunk_size;
}

void*
malloc(size_t size) {
    if (!ctx.is_init) {
        if (!init(&ctx)) return NULL;
    }

    if (size >= MIN_LARGE_SIZE) {
        const u64 chunk_size = calculate_chunk_size(size);
        char* chunk = mmap(NULL, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(chunk)) return NULL;

        ChunkHeader* header = (ChunkHeader*)chunk;
        *header = (ChunkHeader){ .size = chunk_size, .flags = ChunkFlag_Mapped, .user_size = size };
        return chunk + ctx.header_size;
    }

    return 0;
}

void*
realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);

    ChunkHeader* header = get_header(ptr);

    if (header->flags & ChunkFlag_Mapped) {
        if (size <= header->size - metadata_size()) {
            header->user_size = size;
            return ptr;
        };

        const u64 new_chunk_size = calculate_chunk_size(size);
        char* new_chunk = mmap(NULL, new_chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(new_chunk)) return NULL;

        memcopy(new_chunk + ctx.header_size, ptr, header->user_size);
        ChunkHeader* new_header = (ChunkHeader*)new_chunk;
        *new_header = (ChunkHeader){ .size = new_chunk_size, .flags = ChunkFlag_Mapped, .user_size = size };

        munmap(header, header->size);
        return new_chunk + ctx.header_size;
    }

    return NULL;
}

void
free(void* ptr) {
    if (!ptr) return;

    ChunkHeader* header = get_header(ptr);

    if (header->flags & ChunkFlag_Mapped) {
        munmap(header, header->size);
        return;
    }
}

void
show_alloc_mem(void) {
}
