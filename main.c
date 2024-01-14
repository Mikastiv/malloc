#include "memory.h"

int main() {
    char* block = malloc(15000);
    free(block);
}
