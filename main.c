#include "memory.h"

#include <unistd.h>

static void
memoryset(void* dst, const char v, const size_t size) {
    char* dst_ptr = dst;
    for (size_t i = 0; i < size; ++i) {
        dst_ptr[i] = v;
    }
}

size_t
ft_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) ++len;
    return len;
}

void
ft_strcpy(char* dst, const char* src) {
    const size_t len = ft_strlen(src);
    for (size_t i = 0; i < len; ++i) {
        dst[i] = src[i];
    }
    dst[len] = 0;
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

    char* string = malloc(60);
    ft_strcpy(string, "Hello World\n");

    write(1, string, ft_strlen("Hello World\n"));
    string = realloc(string, 90);
    write(1, string, ft_strlen("Hello World\n"));
    free(string);

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


}
