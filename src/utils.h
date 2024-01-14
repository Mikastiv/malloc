#pragma once

#include "types.h"

u64
align_down(const u64 addr, const u64 alignment);

u64
align_up(const u64 addr, const u64 alignment);

void
memcopy(void* dst, const void* src, u64 size);