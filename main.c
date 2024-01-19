#include "memory.h"

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
        blocks[i] = malloc(random() % 8096);
    }
    for (size_t i = 64; i < 128; i++) {
        free(blocks[i]);
    }
    for (size_t i = 64; i < 128; i++) {
        blocks[i] = malloc(random() % 8096);
    }
    show_alloc_mem();
    for (size_t i = 0; i < 128; i++) {
        free(blocks[i]);
    }
    show_alloc_mem();
}
