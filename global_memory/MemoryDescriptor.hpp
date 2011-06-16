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
        coreid_t core_id;

    public:
        MemoryDescriptor();
        ~MemoryDescriptor();

volatile        bool full;
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
       
        void setCoreId( coreid_t ); 
        coreid_t getCoreId();
};

#endif


