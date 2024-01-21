#include "utils.h"

#include <assert.h>
#include <unistd.h>

u64
align_down(const u64 addr, const u64 alignment) {
    assert(alignment != 0);
    assert((alignment & (alignment - 1)) == 0);
    return addr & ~(alignment - 1);
}

u64
align_up(const u64 addr, const u64 alignment) {
    assert(alignment != 0);
    assert((alignment & (alignment - 1)) == 0);
    const u64 mask = alignment - 1;
    return (addr + mask) & ~mask;
}

void
memcopy(void* dst, const void* src, u64 size) {
    char* dst_bytes = (char*)dst;
    const char* src_bytes = (char*)src;

    if (size > 32) {
        u64* dst_big = (u64*)dst;
        const u64* src_big = (u64*)src;

        u64 blocks_count = size / sizeof(u64);
        size = size % sizeof(u64);

        for (u64 i = 0; i < blocks_count; ++i) {
            dst_big[i] = src_big[i];
        }

        dst_bytes += blocks_count * sizeof(u64);
        src_bytes += blocks_count * sizeof(u64);
    }

    for (u64 i = 0; i < size; ++i) {
        for (u64 i = 0; i < size; ++i) {
            dst_bytes[i] = src_bytes[i];
        }
    }
}

bool
mmap_failed(void* ptr) {
    return (u64)ptr == (u64)-1;
}

u64
str_len(const char* str) {
    u64 len = 0;
    while (str[len]) ++len;
    return len;
}

void
putstr(const char* str) {
    write(STDOUT_FILENO, str, str_len(str));
}

void
putnbr(const u64 nbr, const u64 base) {
    const char* hex = "0123456789ABCDEF";
    if (nbr >= base) {
        putnbr(nbr / base, base);
    }
    const char n = hex[nbr % base];
    write(STDOUT_FILENO, &n, 1);
}