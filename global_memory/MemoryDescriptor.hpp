#ifndef __MEMORY_DESCRIPTOR_HPP__
#define __MEMORY_DESCRIPTOR_HPP__

#include "GmTypes.hpp"
#include "thread.h"

#define MD_USE_CACHE_ALIGN 1
#define MD_CACHE_LINE_SIZE_BYTES 64


class MemoryDescriptor {
    private:
        int64_t address; //index into a GA
        uint64_t _data;
        oper_enum operation;
        threadid_t thread_id;
        coreid_t core_id;
        #if MD_USE_CACHE_ALIGN
//char pad1[MD_CACHE_LINE_SIZE_BYTES];
        char pad1[MD_CACHE_LINE_SIZE_BYTES-(sizeof(uint64_t*)+sizeof(uint64_t)+sizeof(oper_enum)+sizeof(threadid_t)+sizeof(coreid_t))];
        #endif

         volatile bool full;
        #if MD_USE_CACHE_ALIGN
       //char pad2[MD_CACHE_LINE_SIZE_BYTES];
       char pad2[MD_CACHE_LINE_SIZE_BYTES-sizeof(bool)];
        #endif
    
    public:
       uint64_t full_poll_count; 


       MemoryDescriptor();
        ~MemoryDescriptor();

        threadid_t getThreadId();
       
        void setThreadId( threadid_t tid) {
            thread_id = tid;
        }

        void setOperation (oper_enum op) {
            operation = op;
        }

        oper_enum getOperation() {
            return operation;
        }

        void setAddress(int64_t addr) {
            address = addr;
        }

        int64_t getAddress() {
            return address;
        }

        bool isFull() {
            return full;
        }

        void setEmpty() {
            full = false;
        }

        void fillData (uint64_t data) {
            // TODO 128-bit write or something like that?
            // For now make assumption that the thing filling is a coroutine in same thread
            
            /*atomic {*/
                _data = data;
                asm volatile("" ::: "memory");
                 // mem barrier
        //        __sync_synchronize ();  unneeded for TSO with compiler barrier
               full = true;
                
            /*}*/
        }

        bool checkData( uint64_t* data) {
            // TODO make sure
            if (full) {
                full = false;
                *data = _data;
                return true;
            } else {
                return false;
            }
        }

        void setData( uint64_t data ) {
            _data = data;
        }

        uint64_t getData () {
            return _data;
        }
           
        void setCoreId(coreid_t cid) {
            core_id = cid;
        }
         
        coreid_t getCoreId() {
            return core_id;
        }

        void setFull() {
            full = true;
        }

        uint64_t* getDataFieldAddress() {
            return &_data;
        }
};

#endif


