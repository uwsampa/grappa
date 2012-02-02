#ifndef __SHMALLOC_H__
#define __SHMALLOC_H__

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

using namespace boost::interprocess;

void* shmalloc(size_t size, const char* key);
void shfree(const char* key);
void* shgetmem(const char* key, int rw);

#endif
