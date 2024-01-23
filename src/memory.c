#include "arena.h"
#include "chunk.h"
#include "freelist.h"
#include "heap.h"
#include "utils.h"

#include <pthread.h>
#include <sys/mman.h>
#include <sys/resource.h>

typedef struct Context {
    Arena arenas[2];
    MappedChunkList mapped_chunks;
    u64 total_memory;
} Context;

static Context ctx;
static pthread_mutex_t mtx;

static int
lock_mutex(void) {
    static bool is_init = false;
    if (!is_init) {
        pthread_mutex_init(&mtx, 0);
        is_init = true;
    }
    return pthread_mutex_lock(&mtx);
}

static int
unlock_mutex(void) {
    return pthread_mutex_unlock(&mtx);
}

static bool
enough_memory(const u64 requested_size) {
    struct rlimit rlp;

    if (getrlimit(RLIMIT_AS, &rlp) == -1) return false;

    return ctx.total_memory + requested_size < rlp.rlim_cur;
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

static void*
get_block(Arena* arena, const u64 requested_size) {
    const u64 size = chunk_unmapped_size(requested_size);

    Chunk* chunk = 0;
    Heap* heap = arena->head;
    while (heap) {
        chunk = heap_find_chunk(heap, size);
        if (chunk) break;
        heap = heap->next;
    }

    if (!chunk) {
        if (!enough_memory(heap_size(arena->type))) return 0;
        if (!arena_grow(arena)) return 0;
        ctx.total_memory += arena->head->size;

        heap = arena->head;
        while (heap) {
            chunk = heap_find_chunk(heap, size);
            if (chunk) break;
            heap = heap->next;
        }
    }

    freelist_remove(&heap->freelist, chunk);

    if (chunk->size - size >= chunk_min_size()) {
        Chunk* other = chunk_split(chunk, size);
        freelist_prepend(&heap->freelist, other);
    }

    chunk->flags |= ChunkFlag_Allocated;
    chunk->user_size = requested_size;

    return chunk_to_mem(chunk);
}

void*
inner_malloc(const u64 size) {
    const u64 mapped_size = chunk_mapped_size(size);

    if (mapped_size >= chunk_min_large_size()) {
        if (!enough_memory(mapped_size)) return 0;
        MappedChunk* mapped = mmap(0, mapped_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(mapped)) return 0;

        ctx.total_memory += mapped_size;

        mapped->next = ctx.mapped_chunks.head;
        ctx.mapped_chunks.head = mapped;

        Chunk* chunk = chunk_from_mapped(mapped);
        chunk->flags = ChunkFlag_Mapped;
        chunk->size = mapped_size;
        chunk->user_size = size;

        return chunk_to_mem(chunk);
    }

    const ArenaType idx = arena_select(chunk_unmapped_size(size));
    return get_block(&ctx.arenas[idx], size);
}

static void
free_chunk(Arena* arena, Chunk* chunk) {
    chunk->flags &= ~ChunkFlag_Allocated;

    Heap* heap = arena_find_heap(arena, chunk);

    Chunk* prev = chunk_prev(chunk);
    if (prev && !chunk_is_allocated(prev)) {
        freelist_remove(&heap->freelist, prev);
        chunk = chunk_coalesce(prev, chunk);
    }

    Chunk* next = chunk_next(chunk);
    if (next && !chunk_is_allocated(next)) {
        freelist_remove(&heap->freelist, next);
        chunk = chunk_coalesce(chunk, next);
    }

    if (chunk->size == heap->size - heap_metadata_size() && arena->len > 1) {
        arena_remove_heap(arena, heap);
        ctx.total_memory -= heap->size;
        munmap(heap, heap->size);
    } else {
        freelist_prepend(&heap->freelist, chunk);
    }
}

void
inner_free(void* ptr) {
    Chunk* chunk = chunk_from_mem(ptr);
    if (chunk_is_mapped(chunk)) {
        MappedChunk* mapped = chunk_to_mapped(chunk);
        remove_mapped_chunk(mapped);
        ctx.total_memory -= chunk->size;
        munmap(mapped, chunk->size);
        return;
    }

    const ArenaType idx = arena_select(chunk->size);
    free_chunk(&ctx.arenas[idx], chunk);
}

void*
inner_realloc(void* ptr, const u64 size) {
    Chunk* chunk = chunk_from_mem(ptr);
    if (chunk_usable_size(chunk) >= size) {
        chunk->user_size = size;
        return ptr;
    }

    // const bool new_size_not_mapped = chunk_mapped_size(size) < chunk_min_large_size();
    // if (!chunk_is_mapped(chunk) && new_size_not_mapped) {
    //     const u64 new_size = chunk_unmapped_size(size);
    //     const bool change_category = chunk->size <= chunk_max_tiny_size() && new_size > chunk_max_tiny_size();
    //     if (!change_category) {
    //         Chunk* next = chunk_next(chunk);
    //         if (next && !chunk_is_allocated(next) && chunk->size + next->size >= new_size) {
    //             Heap* heap = arena_find_heap(&ctx.arenas[ArenaType_Tiny], next);
    //             if (!heap) {
    //                 heap = arena_find_heap(&ctx.arenas[ArenaType_Small], next);
    //             }

    //             freelist_remove(&heap->freelist, next);

    //             chunk = chunk_coalesce(chunk, next);
    //             if (chunk->size - size >= chunk_min_size()) {
    //                 Chunk* other = chunk_split(chunk, size);
    //                 freelist_prepend(&heap->freelist, other);
    //             }

    //             chunk->user_size = size;
    //             return chunk_to_mem(chunk);
    //         }
    //     }
    // }

    void* block = inner_malloc(size);
    memcopy(block, ptr, chunk->user_size);
    inner_free(ptr);

    return block;
}

void*
malloc(size_t size) {
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

static void
print_arena_allocs(const char* name, Arena* arena, u64* total) {
    Heap* heap = arena->head;
    while (heap) {
        putstr(name);
        putstr(" : 0x");
        putnbr((u64)heap_to_chunk(heap), 16);
        putstr("\n");
        Chunk* chunk = heap_to_chunk(heap);
        while (chunk) {
            const u64 addr = (u64)chunk_to_mem(chunk);
            const unsigned long size = chunk->user_size;
            if (chunk_is_allocated(chunk)) {
                *total += size;
                putstr("0x");
                putnbr(addr, 16);
                putstr(" - 0x");
                putnbr(addr + chunk->user_size, 16);
                putstr(" : ");
                putnbr(chunk->user_size, 10);
                putstr(" bytes\n");
            }
            chunk = chunk_next(chunk);
        }
        heap = heap->next;
    }
}

void
show_alloc_mem(void) {
    lock_mutex();
    u64 total = 0;
    print_arena_allocs("TINY", &ctx.arenas[ArenaType_Tiny], &total);
    print_arena_allocs("SMALL", &ctx.arenas[ArenaType_Small], &total);

    MappedChunk* ptr = ctx.mapped_chunks.head;
    while (ptr) {
        Chunk* chunk = chunk_from_mapped(ptr);
        u64 addr = (u64)chunk_to_mem(chunk);
        total += chunk->user_size;
        putstr("LARGE : 0x");
        putnbr((u64)ptr, 16);
        putstr("\n0x");
        putnbr((u64)addr, 16);
        putstr(" - 0x");
        putnbr((u64)addr + chunk->user_size, 16);
        putstr(" : ");
        putnbr(chunk->user_size, 10);
        putstr(" bytes\n");
        ptr = ptr->next;
    }

    putstr("Total : ");
    putnbr(total, 10);
    putstr(" bytes\n");
    unlock_mutex();
}
