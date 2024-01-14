#include "memory.h"

#include "ctx.h"

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

#define CHUNK_ALIGNMENT 16

#ifndef MAX_TINY_SIZE
#define MAX_TINY_SIZE 128
#endif

#ifndef MIN_LARGE_SIZE
#define MIN_LARGE_SIZE 2048
#endif

#if OS_WINDOWS
#error "Windows not supported"
#endif

size_t
align_down(const size_t addr, const size_t alignment) {
    assert(alignment != 0);
    assert((alignment & (alignment - 1)) == 0);
    return addr & ~(alignment - 1);
}

size_t
align_up(const size_t addr, const size_t alignment) {
    assert(alignment != 0);
    assert((alignment & (alignment - 1)) == 0);
    const size_t mask = alignment - 1;
    return (addr + mask) & ~mask;
}



typedef size_t ChunkHeader;

#define CHUNK_HEADER_MASK (~((size_t)(CHUNK_ALIGNMENT - 1)))
#define FLAG_CHUNK_ALLOCATED ((size_t)1 << 0)
#define FLAG_CHUNK_MAPPED ((size_t)1 << 1)

size_t
chunk_size(const ChunkHeader header) {
    return header & CHUNK_HEADER_MASK;
}

ChunkHeader
make_header(const size_t size, size_t flags) {
    assert(align_up(size, CHUNK_ALIGNMENT) == size);
    return size | flags;
}


typedef struct {
    bool is_init;
    size_t page_size;

    char* tiny_chunks;
    char* small_chunks;
} Context;

static Context ctx;

static pthread_mutex_t mtx;

bool
init(Context* ctx) {
    if (ctx->is_init) return true;

    if (pthread_mutex_init(&mtx, NULL) != 0) return false;

    ctx->page_size = (size_t)getpagesize();

    const size_t tiny_alloc_size = align_up(100 * MAX_TINY_SIZE, ctx->page_size);
    const size_t small_alloc_size = align_up(100 * MIN_LARGE_SIZE, ctx->page_size);

    ctx->tiny_chunks = mmap(NULL, tiny_alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    ctx->small_chunks = mmap(NULL, small_alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if ((size_t)ctx->tiny_chunks == (size_t)-1 || (size_t)ctx->small_chunks == (size_t)-1) return false;

    ctx->is_init = true;

    return true;
}

static __attribute__((destructor)) void
deinit(void) {
    if (!ctx.is_init) return;
    pthread_mutex_destroy(&mtx);
}

void*
malloc(size_t size) {
    if (!ctx.is_init) {
        if (!init(&ctx)) return NULL;
    }

    if (size >= MIN_LARGE_SIZE) {
        const size_t user_block_size = align_up(size, CHUNK_ALIGNMENT);
        const size_t header_size = align_up(sizeof(ChunkHeader), CHUNK_ALIGNMENT);
        const size_t chunk_size = align_up(user_block_size + header_size, CHUNK_ALIGNMENT);
        char* chunk = mmap(NULL, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if ((size_t)chunk == (size_t)-1) return NULL;

        ChunkHeader* header = (ChunkHeader*)chunk;
        *header = make_header(user_block_size, FLAG_CHUNK_MAPPED);
        return chunk + header_size;
    }

    return 0;
}

void*
realloc(void* ptr, size_t size) {
    (void)ptr;
    (void)size;
    return 0;
}

void
free(void* ptr) {
    if (!ptr) return;

    char* chunk = ptr;
    ChunkHeader* header = (ChunkHeader*)(chunk - CHUNK_ALIGNMENT);

    if (*header & FLAG_CHUNK_MAPPED) {
        munmap(header, chunk_size(*header));
        return;
    }
}

void
show_alloc_mem(void) {
}
