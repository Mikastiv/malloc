#include "memory.h"

#include <unistd.h>

static void
memoryset(void* dst, const char v, const size_t size) {
    char* dst_ptr = dst;
    for (size_t i = 0; i < size; ++i) {
        dst_ptr[i] = v;
    }
}

void
memcopy(void* dst, const void* src, const size_t size) {
    char* dst_ptr = dst;
    const char* src_ptr = src;
    for (size_t i = 0; i < size; ++i) {
        dst_ptr[i] = src_ptr[i];
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
randomnumber(void) {
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
    for (size_t i = 0; i < 256; i++) {
        const size_t size = randomnumber() % 1024 * 64;
        blocks[i] = malloc(size);
        memoryset(blocks[i], 0xAA, size);
    }
    for (size_t i = 128; i < 256; i++) {
        free(blocks[i]);
    }
    for (size_t i = 0; i < 128; i++) {
        const size_t size = randomnumber() % 1024 * 64;
        blocks[i] = realloc(blocks[i], size);
        memoryset(blocks[i], 0xAA, size);
    }
    for (size_t i = 128; i < 256; i++) {
        const size_t size = randomnumber() % 1024 * 64;
        blocks[i] = malloc(size);
        memoryset(blocks[i], 0xAA, size);
    }
    show_alloc_mem();
    for (size_t i = 0; i < 256; i++) {
        free(blocks[i]);
    }
    free(blocks);
    show_alloc_mem();

    char* string = malloc(60);
    memcopy(string, "Hello World\n", 13);

    write(1, string, 13);
    string = realloc(string, 90);
    write(1, string, 13);
}
