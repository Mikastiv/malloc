#include "memory.h"

#include "arena.h"
#include "chunk.h"
#include "defines.h"
#include "freelist.h"
#include "types.h"
#include "utils.h"

#include <pthread.h>
#include <stdbool.h>
#include <sys/mman.h>

typedef struct Context {
    bool is_init;
    Arena arena_tiny;
    Arena arena_small;
    Freelist freelist_tiny;
    Freelist freelist_small;
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

static char*
get_block(Arena* arena, Freelist* list, const u64 requested_size) {
    // get from freelist
    char* block = freelist_get_block(list, requested_size);
    if (block) return block;

    // get from top of heap
    block = heap_get_block(arena->head, requested_size);
    if (block) return block;

    // grow heap and get block
    if (!arena_grow(arena)) return 0;
    block = heap_get_block(arena->head, requested_size);

    return block;
}

static void*
inner_malloc(size_t size) {
    char* block = 0;
    if (chunk_calculate_size(size, true) >= MIN_LARGE_SIZE) {
        const u64 chunk_size = chunk_calculate_size(size, true);
        char* chunk = mmap(0, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (mmap_failed(chunk)) return 0;

        MappedChunk* mapped = (MappedChunk*)chunk;
        mapped->next = ctx.mapped_chunks.head;
        ctx.mapped_chunks.head = mapped;

        ChunkHeader* header = chunk_get_header_from_mapped(mapped);
        *header = (ChunkHeader){ .size = chunk_size, .flags = ChunkFlag_Mapped, .user_size = size };

        block = chunk_data_start(header);
    } else if (chunk_calculate_size(size, false) <= MAX_TINY_SIZE) {
        block = get_block(&ctx.arena_tiny, &ctx.freelist_tiny, size);
    } else {
        block = get_block(&ctx.arena_small, &ctx.freelist_small, size);
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

static void
remove_mapped_chunk(ChunkHeader* header) {
    MappedChunk* chunk = (MappedChunk*)((char*)header - chunk_mapped_header_size());
    MappedChunk* ptr = ctx.mapped_chunks.head;

    if (chunk == ptr) {
        ctx.mapped_chunks.head = ptr->next;
        return;
    }

    MappedChunk* tmp = ptr;
    ptr = ptr->next;
    while (ptr) {
        if (chunk == ptr) {
            tmp->next = ptr->next;
            return;
        }
        tmp = ptr;
        ptr = ptr->next;
    }
}

static void
free_block(Freelist* list, ChunkHeader* header) {
    ChunkHeader* footer = chunk_get_footer(header);
    header->flags &= ~ChunkFlag_Allocated;
    footer->flags &= ~ChunkFlag_Allocated;

    ChunkHeader* prev = chunk_prev(header);
    if (prev && (prev->flags & ChunkFlag_Allocated) == 0) {
        header = chunk_coalesce(prev, header);
    }

    ChunkHeader* next = chunk_next(header);
    if (next && (next->flags & ChunkFlag_Allocated) == 0) {
        header = chunk_coalesce(header, next);
    }

    freelist_prepend(list, header);
}

static void
inner_free(void* ptr) {
    ChunkHeader* header = chunk_get_header(ptr);

    if (header->flags & ChunkFlag_Mapped) {
        remove_mapped_chunk(header);
        munmap(header, header->size);
    } else if (chunk_calculate_size(header->size, false) <= MAX_TINY_SIZE) {
        free_block(&ctx.freelist_tiny, header);
    } else {
        free_block(&ctx.freelist_small, header);
    }
}

void
free(void* ptr) {
    if (!ptr) return;

    lock_mutex();
    inner_free(ptr);
    unlock_mutex();
}

static void*
inner_realloc(void* ptr, size_t size) {
    void* block = 0;

    ChunkHeader* header = chunk_get_header(ptr);
    if (size <= header->size - chunk_metadata_size(header->flags & ChunkFlag_Mapped)) {
        header->user_size = size;
        block = ptr;
    } else {
        block = inner_malloc(size);
        memcopy(block, ptr, header->user_size);
        inner_free(ptr);
    }

    return block;
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
        putnbr((u64)heap_data_start(heap), 16);
        putstr("\n");
        ChunkHeader* chunk = heap_data_start(heap);
        while (chunk) {
            const u64 addr = (u64)chunk_data_start(chunk);
            const unsigned long size = chunk->user_size;
            *total += size;
            if (chunk->flags & ChunkFlag_Allocated) {
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
    u64 total = 0;
    print_arena_allocs("TINY", &ctx.arena_tiny, &total);
    print_arena_allocs("SMALL", &ctx.arena_small, &total);

    MappedChunk* ptr = ctx.mapped_chunks.head;
    while (ptr) {
        ChunkHeader* header = chunk_get_header_from_mapped(ptr);
        u64 addr = (u64)chunk_data_start(header);
        total += header->user_size;
        putstr("LARGE : ");
        putnbr((u64)ptr, 16);
        putstr("\n0x");
        putnbr((u64)addr, 16);
        putstr(" - 0x");
        putnbr((u64)addr + header->user_size, 16);
        putstr(" : ");
        putnbr(header->user_size, 10);
        putstr(" bytes\n");
        ptr = ptr->next;
    }

    putstr("Total : ");
    putnbr(total, 10);
    putstr(" bytes\n");
}
