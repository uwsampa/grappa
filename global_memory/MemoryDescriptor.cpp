#include "MemoryDescriptor.hpp"
#include "thread.h"

MemoryDescriptor::MemoryDescriptor() {
    address = 0x0;
    _data = 0;
    operation = READ;
    full = false;
    thread_id = -1;

    full_poll_count = 0;
}

MemoryDescriptor::~MemoryDescriptor() { }

