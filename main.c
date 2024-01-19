#include "memory.h"

static void
memoryset(void* dst, const size_t v, size_t size) {
    char* dst_bytes = (char*)dst;

    if (size > 32) {
        size_t* dst_big = (size_t*)dst;

        size_t blocks_count = size / sizeof(size_t);
        size = size % sizeof(size_t);

        for (size_t i = 0; i < blocks_count; ++i) {
            dst_big[i] = v;
        }

        dst_bytes += blocks_count * sizeof(size_t);
    }

    for (size_t i = 0; i < size; ++i) {
        for (size_t i = 0; i < size; ++i) {
            dst_bytes[i] = (char)v;
        }
    }
}

static size_t
rotate_left(const size_t n, const size_t d) {
    static const size_t nbits = sizeof(size_t) * 8;

    const size_t a = n << d;
    const size_t b = n >> (nbits - d);
    return a | b;
}

static size_t
random(void) {
    static size_t state[4] = {23, 912349023, 123612, 1029346218764};

    const size_t r = rotate_left(state[0] + state[3], 23) + state[0];
    const size_t t = state[1] << 17;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];
    state[2] ^= t;
    state[3] = rotate_left(state[3], 45);

    return r;
}

int main() {
    void** blocks = malloc(256 * sizeof(void*));
    for (size_t i = 0; i < 128; i++) {
        const size_t size = random() % 8096;
        blocks[i] = malloc(size);
        memoryset(blocks[i], 0xAAAAAAAABBBBBBBBULL, size);
    }
    for (size_t i = 64; i < 128; i++) {
        free(blocks[i]);
    }
    for (size_t i = 0; i < 64; i++) {
        const size_t size = random() % 8096;
        blocks[i] = realloc(blocks[i], size);
        memoryset(blocks[i], 0xAAAAAAAABBBBBBBBULL, size);
    }
    for (size_t i = 64; i < 128; i++) {
        const size_t size = random() % 8096;
        blocks[i] = malloc(size);
        memoryset(blocks[i], 0xAAAAAAAABBBBBBBBULL, size);
    }
    show_alloc_mem();
    for (size_t i = 0; i < 128; i++) {
        free(blocks[i]);
    }
    free(blocks);
    show_alloc_mem();
}
