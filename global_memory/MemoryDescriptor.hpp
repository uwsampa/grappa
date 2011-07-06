#ifndef __MEMORY_DESCRIPTOR_HPP__
#define __MEMORY_DESCRIPTOR_HPP__

#include "GmTypes.hpp"
#include "thread.h"

#define MD_USE_CACHE_ALIGN 0
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
       MemoryDescriptor();
        ~MemoryDescriptor();

        void fillData( uint64_t data );
        bool checkData( uint64_t* data);
        void setEmpty();
        void setFull();
        void setData( uint64_t data);
        bool isFull(); //XXX temp
        uint64_t getData();
        uint64_t* getDataFieldAddress();

        void setAddress( int64_t addr);
        int64_t getAddress();

        void setOperation (oper_enum op);
        oper_enum getOperation();

        void setThreadId( threadid_t );
        threadid_t getThreadId();
       
        void setCoreId( coreid_t ); 
        coreid_t getCoreId();
};

#endif


