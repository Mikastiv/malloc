#include "memory.h"

int main() {
    char* block = malloc(64);
    free(block);
}
