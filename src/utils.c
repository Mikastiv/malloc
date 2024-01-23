#include "utils.h"

#include <unistd.h>

u64
align_down(const u64 addr, const u64 alignment) {
    return addr & ~(alignment - 1);
}

u64
align_up(const u64 addr, const u64 alignment) {
    const u64 mask = alignment - 1;
    return (addr + mask) & ~mask;
}

void
memcopy(void* dst, const void* src, const u64 size) {
    char* dst_ptr = dst;
    const char* src_ptr = src;
    for (size_t i = 0; i < size; ++i) {
        dst_ptr[i] = src_ptr[i];
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
