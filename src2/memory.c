#include "memory.h"

#include "arena.h"
#include "utils.h"

#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>

typedef struct Context {
    bool is_init;
    Arena arena_tiny;
    Arena arena_small;
    MappedChunkList mapped_chunks;
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

    if (pthread_mutex_init(&mtx, 0) != 0) return false;

    ctx.is_init = true;

    return true;
}

static __attribute__((destructor)) void
deinit(void) {
    if (!ctx.is_init) return;
    pthread_mutex_destroy(&mtx);
}

static void
remove_mapped_chunk(MappedChunk* mapped) {
    MappedChunk* ptr = ctx.mapped_chunks.head;

    if (mapped == ptr) {
        ctx.mapped_chunks.head = ptr->next;
        return;
    }

    MappedChunk* tmp = ptr;
    ptr = ptr->next;
    while (ptr) {
        if (mapped == ptr) {
            tmp->next = ptr->next;
            return;
        }
        tmp = ptr;
        ptr = ptr->next;
    }
}

void*
inner_malloc(const u64 size) {
    void* block = 0;

    const u64 mapped_size = chunk_calculate_size(size, true);
    const u64 unmapped_size = chunk_calculate_size(size, false);

    if (mapped_size >= chunk_min_large_size()) {
        MappedChunk* mapped = mmap(0, mapped_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(mapped)) return 0;

        mapped->next = ctx.mapped_chunks.head;
        ctx.mapped_chunks.head = mapped;

        Chunk* chunk = chunk_from_mapped(mapped);
        chunk->flags = ChunkFlag_Mapped;
        chunk->size = mapped_size;
        chunk->user_size = size;

        block = chunk_to_mem(chunk);
    } else if (unmapped_size <= chunk_min_size()) {

    } else {

    }

    return block;
}

static void
free_chunk(Arena* arena, Chunk* chunk) {
    chunk->flags &= ~ChunkFlag_Allocated;

    Chunk* prev = chunk_prev(chunk);
    if (prev && (prev->flags & ChunkFlag_Allocated) == 0) {
        chunk = chunk_coalesce(prev, chunk);
    }

    Chunk* next = chunk_next(chunk);
    if (next && (next->flags & ChunkFlag_Allocated) == 0) {
        chunk = chunk_coalesce(chunk, next);
    }

    Heap* heap = arena_find_heap(arena, chunk);
    freelist_prepend(&heap->freelist, chunk);
}

void
inner_free(void* ptr) {
    Chunk* chunk = chunk_from_mem(ptr);
    if (chunk->flags & ChunkFlag_Mapped) {
        MappedChunk* mapped = chunk_to_mapped(chunk);
        remove_mapped_chunk(mapped);
        munmap(mapped, chunk->size);
    } else if (chunk->size <= chunk_min_size()) {
        free_chunk(&ctx.arena_tiny, chunk);
    } else {
        free_chunk(&ctx.arena_small, chunk);
    }
}

void*
inner_realloc(void* ptr, const u64 size) {
    void* block = 0;

    Chunk* chunk = chunk_from_mem(ptr);
    if (chunk_usable_size(chunk) >= size) {
        chunk->user_size = size;
        block = ptr;
    } else {
        // TODO: check for free block after
        block = inner_malloc(size);
        memcopy(block, ptr, chunk->user_size);
        inner_free(ptr);
    }

    return block;
}

void*
malloc(size_t size) {
    if (!ctx.is_init) {
        if (!init()) return 0;
    }

    lock_mutex();
    void* block = inner_malloc(size);
    unlock_mutex();

    return block;
}

void
free(void* ptr) {
    if (!ptr) return;

    lock_mutex();
    inner_free(ptr);
    unlock_mutex();
}

void*
realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (!size) {
        free(ptr);
        return 0;
    }

    lock_mutex();
    void* block = inner_realloc(ptr, size);
    unlock_mutex();

    return block;
}

void
show_alloc_mem(void);
