#pragma once

#include "types.h"

#include <stdbool.h>

u64
align_down(const u64 addr, const u64 alignment);

u64
align_up(const u64 addr, const u64 alignment);

void
memcopy(void* dst, const void* src, const u64 size);

bool
mmap_failed(void* ptr);

u64
str_len(const char* str);

void
putstr(const char* str);

void
putnbr(const u64 nbr, const u64 base);
