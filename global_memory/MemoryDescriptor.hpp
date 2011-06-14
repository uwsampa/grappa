#ifndef __MEMORY_DESCRIPTOR_HPP__
#define __MEMORY_DESCRIPTOR_HPP__

#include "GmTypes.hpp"
#include "thread.h"

class MemoryDescriptor {
    private:
        uint64_t* address;
        uint64_t _data;
        oper_enum operation;

        threadid_t thread_id;

    public:
        MemoryDescriptor();
        ~MemoryDescriptor();

        bool full;
        void fillData( uint64_t data );
        bool checkData( uint64_t* data);
        void setEmpty();
        void setData( uint64_t data);
        bool isFull(); //XXX temp
        uint64_t getData();

        void setAddress( uint64_t* addr);
        uint64_t* getAddress();

        void setOperation (oper_enum op);
        oper_enum getOperation();

        void setThreadId( threadid_t );
        threadid_t getThreadId();
        

};

MemoryDescriptor::MemoryDescriptor() {
    address = 0x0;
    _data = 0;
    operation = READ;
    full = false;
    thread_id = -1;
}

void MemoryDescriptor::setThreadId( threadid_t tid) {
    thread_id = tid;
}

void MemoryDescriptor::setOperation (oper_enum op) {
    operation = op;
}

void MemoryDescriptor::setAddress(uint64_t* addr) {
    address = addr;
}

uint64_t* MemoryDescriptor::getAddress() {
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
    
        
#endif


