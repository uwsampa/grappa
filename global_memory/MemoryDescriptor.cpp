#include "MemoryDescriptor.hpp"
#include "thread.h"

MemoryDescriptor::MemoryDescriptor() {
    address = 0x0;
    _data = 0;
    operation = READ;
    full = false;
    thread_id = -1;
}

MemoryDescriptor::~MemoryDescriptor() { }

void MemoryDescriptor::setThreadId( threadid_t tid) {
    thread_id = tid;
}

void MemoryDescriptor::setOperation (oper_enum op) {
    operation = op;
}

oper_enum MemoryDescriptor::getOperation() {
    return operation;
}

void MemoryDescriptor::setAddress(int64_t addr) {
    address = addr;
}

int64_t MemoryDescriptor::getAddress() {
    return address;
}

bool MemoryDescriptor::isFull() {
    return full;
}

void MemoryDescriptor::setEmpty() {
    full = false;
}

void MemoryDescriptor::fillData (uint64_t data) {
    // TODO 128-bit write or something like that?
    // For now make assumption that the thing filling is a coroutine in same thread
    
    /*atomic {*/
        _data = data;
        asm volatile("" ::: "memory");
         // mem barrier
        __sync_synchronize ();
       full = true;
        
    /*}*/
}

bool MemoryDescriptor::checkData( uint64_t* data) {
    // TODO make sure
    if (full) {
        full = false;
        *data = _data;
        return true;
    } else {
        return false;
    }
}

void MemoryDescriptor::setData( uint64_t data ) {
    _data = data;
}

uint64_t MemoryDescriptor::getData () {
    return _data;
}
   
void MemoryDescriptor::setCoreId(coreid_t cid) {
    core_id = cid;
}
 
coreid_t MemoryDescriptor::getCoreId() {
    return core_id;
}

void MemoryDescriptor::setFull() {
    full = true;
}

uint64_t* MemoryDescriptor::getDataFieldAddress() {
    return &_data;
}
