#include "freelist.h"

char*
freelist_get_block(Freelist* list, const u64 requested_size) {
    (void)list;
    const u64 size = chunk_calculate_size(requested_size, false);
    (void)size;

    return 0;
}
