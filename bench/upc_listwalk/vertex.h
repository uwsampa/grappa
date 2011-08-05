#include <stdint.h>

typedef struct vertex {
    uint64_t id;
    struct vertex* next;
} vertex;
