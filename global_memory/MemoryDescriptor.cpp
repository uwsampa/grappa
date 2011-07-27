#include "MemoryDescriptor.hpp"
#include "thread.h"


/*
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

char* sprintit(int x) {
    char* buf = (char*)malloc(64);
    sprintf(buf, "core%d", x);
    return buf;
}*/

MemoryDescriptor::MemoryDescriptor()
   // : latency_timer(sprintit(omp_get_thread_num()), 2, 0, false) {
    : latency_timer("thr_latency", 2, 0, false) {
    address = 0x0;
    _data = 0;
    operation = READ;
    full = false;
    thread_id = -1;

    full_poll_count = 0;
      
}

MemoryDescriptor::~MemoryDescriptor() { }

