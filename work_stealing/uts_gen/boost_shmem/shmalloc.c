#include "shmalloc.h"

#include <iostream>
#include <cstring>


using namespace boost::interprocess;

void* shmalloc(size_t size, const char* key) {
    try {
        shared_memory_object* shm = new shared_memory_object(create_only, key, read_write);
        shm->truncate(size);
        mapped_region* region = new mapped_region(*shm, read_write);
        void* endaddress = (void*) (((char*)region->get_address())+region->get_size());
        std::cout << "mapped size = " << region->get_size() << " / " << size << std::endl;
        std::cout << "mapped reg: [" << std::hex << region->get_address() << "," << endaddress << ")" << std::endl;
        return region->get_address();
    } catch(interprocess_exception &ex) {
        std::cout << "Unexpected exception (allocating " << key <<  "):" << ex.what() << std::endl;
        return 0;
    }
}

//todo free the region object
void shfree(const char* key) {
    shared_memory_object::remove(key);
}

// 1 read_write, 0 read_only
void* shgetmem(const char* key, int rw) {
    try {
        if (rw) {
            shared_memory_object* shm = new shared_memory_object(open_only, key, read_write);
            mapped_region* region = new mapped_region(*shm, read_write);
            return region->get_address();
        } else {
            shared_memory_object* shm = new shared_memory_object(open_only, key, read_only);
            mapped_region* region = new mapped_region(*shm, read_only);
            return region->get_address();
        }
    } catch(interprocess_exception &ex) {
        std::cout << "Unexpected exception (opening " << key << "):" << ex.what() << std::endl;
        return 0;
    }
}




//string name_prefix = "_shmalloc_";
//int next_guid = 0;
//
//typedef struct shmem_object {
//    void* ptr;
//    int guid;
//} shmem_t;

//shmem_t shmalloc(size_t size, const char* name) {
//   shmem_t s;
//   s.guid = next_guid++;
//   string name = name_prefix;
//   name.append(itoa(s.guid));
//   shared_memory_object shm (create_only, name, read_write);
//   shm.truncate(size);
//   mapped_region region(shm, read_write);
//   s.ptr = region.get_address();
//   return s;
//}

//void shfree(shmem_t s) {
//    string name = name_prefix;
//    name.append(itoa(s.guid));
//    shared_memory_object::remove(name);
//}
