/*
 * $Id: allocator.cpp,v 1.1 2001/10/21 03:19:57 root Exp root $
 */
/* 
 * allocate() uses mmap() instead of pvalloc()
 */
/* #define USE_MMAP 1 
 */

/*
 * Intercept (using ld --wrap feature) free and mprotect 
 */
/* #define USE_WRAP 1 
 */
#include "includes.h"
#include "allocator.h"

using namespace std;
using namespace STLMemDebug;

#ifdef UNIX
///////////////////////////////////////////////////////////////
// Unix-specific section
 #include <malloc.h>
 #include <unistd.h>
 #include <sys/mman.h>
 
static const bool use_mmap =
 #if USE_MMAP
    true;
 #else
    false;
 #endif

#if USE_WRAP
 // --wrap symbol 
 // GNU linker: Use a wrapper function for symbol.
 // Any undefined reference to symbol will be
 // resolved  to __wrap_symbol. Any undefined reference
 // to __real_symbol will be resolved to symbol.
 
 extern "C" int  __real_mprotect(const void*, size_t, int);
 extern "C" void __wrap_free(void*);
 extern "C" void __real_free(void*);

 #define MPROTECT __real_mprotect
 #define FREE __real_free
#else
 #define MPROTECT mprotect
 #define FREE free
#endif // USE_WRAP

// Translate enumerated type to old, C-style flags
static int accessMode2Int(AccessMode mode)
{
    int flags(0);

    switch (mode)
    {
    case memNoAccess: flags = PROT_NONE;break;
    case memReadOnly: flags = PROT_READ; break;
    case memReadWrite:flags = PROT_READ | PROT_WRITE; break;
    }
    return flags;
}

#elif defined(WIN32)
 //////////////////////////////////////////////////////////////
 // Win32-specific section
 #ifndef STRICT
  #define STRICT
 #endif
 #include <windows.h>

 // Translate enumerated type to old, C-style flags
 static DWORD accessMode2Int(AccessMode mode) 
 {
     DWORD flags(0);

     switch (mode)
     {
     case memNoAccess: flags = PAGE_NOACCESS; break;
     case memReadOnly: flags = PAGE_READONLY; break;
     case memReadWrite:flags = PAGE_READWRITE;break;
     }
     return flags;
 }
#endif // WIN32

#if defined(__SGI_STL_ALGORITHM)
    // The version of SGI STL shipped with
    // RedHat 7.1 does not have <limits>?
    size_t STLMemDebug::size_t_max()
    { return UINT_MAX; }
#else
 
 // Avoid clash with macro max in MSVC 6
#ifdef max
 #undef max
#endif
    
    size_t STLMemDebug::size_t_max()
    { return numeric_limits<size_t>::max(); }
#endif

///////////////////////////////////////////////////////////////
// Local functions and classes
//
namespace
{
    /**
     * Changes the access protection mode on a region of 
     * memory in the virtual address space of the calling 
     * process. 
     */
    bool protectMemory
    (
        const void* ptr,    // points to memory area
        size_t size,        // size in bytes of the area
        AccessMode mode     // desired access mode
    )
    {
        const int flags = accessMode2Int(mode);
        bool result = true;

        if (size)
        {
            // system APIs are not const-correct!
            void* addr = const_cast<void*>(ptr);

    #ifdef UNIX
            if (MPROTECT(addr, size, flags) < 0)
            {
                cerr << "protectMemory failed: ";
                cerr << ::strerror(errno) << endl;

                result = false;
            }
    #elif defined (WIN32)
    
            DWORD oldAccess(0);

            if (!::VirtualProtect(addr, size, flags, &oldAccess))
            {
                cerr << "protectMemory failed: ";
                cerr << ::GetLastError() << endl;

                result = false;
            }
    #endif // WIN32
        }
        return result;
    }

    /**
     * Concrete memory manager implementation.
     * In a multi-threaded model, we want to
     * synchronize the access to the internal
     * list of memory block.
     */
    class MemMgrImpl 
        // implements the MemMgr interface
        : public MemMgr 

        // inherits from CriticalSection to take
        // advantage of the "empty base class" 
        // optimization in a single-thread model
        , public CriticalSection
    {
    public:
        MemMgrImpl() : internal_(false)
        {
            // clog << "MemMgrImpl::MemMgrImpl()" << endl;
        }

        ~MemMgrImpl()
        {
            // clog << "MemMgrImpl::~MemMgrImpl()" << endl;
            //
            // the vector's destruction
            // may invoke deallocate()
            setInternal(true); 
        }

        void protect(AccessMode);
        
        void setInternal(bool flag)
        { internal_ = flag; }
        
        bool isInternal() const
        { return internal_; }

    private:
        // A couple of helper inner classes:
        /**
         * Memory Block Descriptor. Has info 
         * about the allocated memory blocks.
         */
        class MemBlock
        {
        public:
            MemBlock(const void* addr, size_t size)
                : addr_(addr)
                , size_(size)
            {}
            
            MemBlock(const MemBlock& that) 
                : addr_(that.addr_)
                , size_(that.size_)
            {}

            MemBlock& operator=(const MemBlock& rhs);

            const void* getAddr() const
            { return addr_; }

            size_t getSize() const
            { return size_; }
        
            // Sets access mode for this block
            bool setAccessMode(AccessMode);

            // For sorting memory blocks 
            // by their addresses
            bool operator<(const MemBlock& rhs) const
            { return getAddr() < rhs.getAddr(); }

            bool operator==(const MemBlock& rhs) const
            { return getAddr() == rhs.getAddr(); }
            
        private:
            const void* addr_;
            size_t size_;
        }; // class MemBlock

        // Defines an internal operation scope.
        // See comments for internal_ below.
        // 
        // Inherits from Lock to take advantage
        // of the "empty base class" optimization
        // when compiled as single-threaded
        class InternalScope : public Lock
        {
        public:
            explicit InternalScope(MemMgrImpl* pMemMgr)
                : Lock(*pMemMgr)
                , pMemMgr_(pMemMgr)
            {
                pMemMgr_->protect(memReadWrite);
                pMemMgr_->setInternal(true);
            }

            ~InternalScope()
            {
                pMemMgr_->protect(memReadOnly);
                pMemMgr_->setInternal(false);
            }

        private:
            MemMgrImpl* pMemMgr_;
        };


        // Sets access mode for all 
        // managed memory blocks
        virtual void setAccessMode(AccessMode);

        // Inserts block info into the 
        // internal list of mem blocks
        virtual void insert(const void*, size_t);

        // Remove memory block info
        virtual void erase(const void*, size_t);

        // Returns the number of managed blocks
        virtual size_t size() const
        { return memBlockList_.size(); }

        // Prints allocated memory 
        // block info to logging device
        virtual void dumpBlocks() const;
        
        // Returns true if there is a
        // mem block starting at address
        virtual bool search(const void* addr) const;

    private:
        // On UNIX, the memory may need to
        // be aligned on a page boundary in order for
        // mprotect() to work; we use the Allocator
        typedef vector<MemBlock, 
            Allocator<MemBlock> > MemBlockList;

        MemBlockList memBlockList_;

        // The Manager uses the Allocator, which in turn
        // uses the manager; we use this flag to avoid 
        // infinite re-entry
        bool internal_;
    }; // class MemMgrImpl

} // namespace

MemMgr* BaseAllocator::pMgr_ = 0;

MemMgr& BaseAllocator::getMemMgr()
{
    if (pMgr_ == 0)
    {
        pMgr_ = new MemMgrImpl();
    }
    return *pMgr_;
}


/**************************************************************
 * Allocates size bytes
 */
void* STLMemDebug::allocate(size_t size)
{
    void *pv = 0;

#if defined(WIN32)

    pv = ::VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
    
#elif defined(UNIX)

    if (use_mmap)
    {
        pv = ::mmap(0, size, (PROT_READ | PROT_WRITE), 
            (MAP_PRIVATE | MAP_ANONYMOUS), -1, 0);

        // mprotect seems to be working on Linux for
        // memory allocated with pvalloc; if it doesn't
        // work on your system, turn the use_mmap flag
        // on.
        // Linux manual page for mprotect(2): "POSIX.1b 
        // says  that  mprotect can be used only on 
        // regions of memory obtained from mmap(2)".
        
        if (pv == reinterpret_cast<void*>(-1))
        {
            throw bad_alloc(); // mmap failed
        }
    }
    else
    {
        // allocate memory aligned to a page boundary
        pv = ::pvalloc(size);
        
        if (pv == 0)
        {
            throw bad_alloc(); // pvalloc failed
        }
    }
#endif // UNIX

    return pv;
}

/**************************************************************
 * Free memory at address ptr, of given size (in bytes)
 */
void STLMemDebug::deallocate(void* ptr, size_t size) throw()
{
#if defined(WIN32)

    ::VirtualFree(ptr, size, MEM_DECOMMIT);

#elif defined(UNIX)
    if (use_mmap)
    {
        if (::munmap(ptr, size) != 0)
        {
            cerr << "deallocate(): munmap failed" << endl;
            abort();
        }
    }
    else
    {
        FREE(ptr);
    }
#endif // UNIX
}

/**************************************************************
 * Calculate the amount of memory needed to store nObjects. 
 * Round memory up to a multiple of page size as needed.
 */
size_t STLMemDebug::calcMemSize(size_t nObjects, size_t objectSize)
{
    size_t size = nObjects * objectSize;

#if defined(UNIX) && defined(USE_MMAP)
    // round up to multiple of page size
    const size_t pageSize = ::getpagesize();
    // return ((size + pageSize - 1) / pageSize) * pageSize;
    return (size + pageSize - 1) & ~(pageSize - 1);
#else
    return size;
#endif
}

/**************************************************************
 *
 */
MemMgrImpl::MemBlock&
MemMgrImpl::MemBlock::operator=(const MemBlock& rhs)
{
    MemBlock tmp(rhs);
    std::swap(tmp.addr_, addr_);
    std::swap(tmp.size_, size_);

    return *this;
}

/**************************************************************
 * Set access (protection) mode for this memory block. 
 */
bool MemMgrImpl::MemBlock::setAccessMode(AccessMode mode)
{
    return protectMemory(getAddr(), getSize(), mode);
}

/**************************************************************
 * Set access mode for internal data structures 
 */
void MemMgrImpl::protect(AccessMode mode)
{
    // calculate the size of our internal list, in bytes
    const size_t size = memBlockList_.size() * sizeof(MemBlock);
    
    protectMemory(&(memBlockList_[0]), size, mode);
}

/**************************************************************
 * Returns true if the memory pointed onto by ptr
 * is owned by our memory manager
 */
bool MemMgrImpl::search(const void* ptr) const
{
    Lock syncThreads(*this);

#if 1 // FAST

    MemBlock tmp(ptr, 0);

    return binary_search(memBlockList_.begin(), 
        memBlockList_.end(), tmp);

#else // THOROUGH
    
    MemBlockList::const_iterator i(memBlockList_.begin());
    for (; i != memBlockList_.end(); ++i)
    {
        const char* addr = reinterpret_cast<const char*>(i->getAddr()); 
        if (ptr >= addr && ptr < (addr + i->getSize()))
        {
            return true;
        }
    }
#endif 
    return false;
}


/**************************************************************
 * Register memory block with the Memory Manager
 */
void MemMgrImpl::insert(const void* ptr, size_t size)
{
    if (isInternal())  // avoid infinite re-entry
    {
        return;
    }
    {
        InternalScope scope(this);
        MemBlock mblock(ptr, size);
    
        // insert the memory block, keeping the vector sorted 
        // in ascending order of addresses
        MemBlockList::iterator i = upper_bound(
            memBlockList_.begin(), memBlockList_.end(), mblock);

        memBlockList_.insert(i, mblock);
    }

    clog << "allocated " << size 
         << " bytes at " << hex << ptr << dec << endl;
}

/**************************************************************
 * Deregister memory block with the Memory Manager
 */
void MemMgrImpl::erase(const void* ptr, size_t size)
{
    if (isInternal())  // avoid infinite re-entry
    {
        return;
    }
    {
        InternalScope scope(this);
    
        MemBlock mblock(ptr, size);
        MemBlockList::iterator i = lower_bound(
            memBlockList_.begin(), memBlockList_.end(), mblock);

        /*
        if (i == memBlockList_.end())
        {
            cerr << "deregisterBlock(): invalid address" << endl;
            abort();
        }
        if (i->getSize() != size)
        {
            cerr << "deregisterBlock(): invalid block size" << endl;
            abort();
        }
        */
        assert(i != memBlockList_.end());
        assert(i->getSize() == size);

        memBlockList_.erase(i);
    }
    clog << "deallocated " << size 
         << " bytes at "   << hex << ptr << dec << endl;
}


/**************************************************************
 * Iterates through all allocated memory blocks and 
 * sets the specified access mode
 */
void MemMgrImpl::setAccessMode(AccessMode mode)
{
    Lock syncThreads(*this);

    MemBlockList::iterator i(memBlockList_.begin());
    for (; i != memBlockList_.end(); ++i)
    {
        i->setAccessMode(mode);
    }
}

/**************************************************************
 *
 */
void MemMgrImpl::dumpBlocks() const
{
    Lock syncThreads(*this);

    MemBlockList::const_iterator i(memBlockList_.begin());
    for (; i != memBlockList_.end(); ++i)
    {
        clog << hex << i->getAddr() << dec;
        clog << " size=" << i->getSize() << endl;
    }
}

#if USE_WRAP
/**************************************************************
 * Make sure that nobody is tampering with our memory by 
 * directly free-ing it or changing the protection mode
 *
 * Requires GNU linker (or a linker that supports the --wrap
 * command line option)
 */
extern "C" void
__wrap_free(void* ptr)
{
    if (BaseAllocator::getMemMgr().search(ptr))
    {
        cerr << "free(" << ptr << "): not owner" << endl;
        abort();
    }
    __real_free(ptr);
}

extern "C" int
__wrap_mprotect(const void* ptr, size_t len, int prot)
{
    if (BaseAllocator::getMemMgr().search(ptr))
    {
        cerr << "mprotect(" << ptr << ", " << len << ", 0x" 
             << hex << prot << dec << "): not owner" << endl;
        abort();
    }
    return __real_mprotect(ptr, len, prot);
}
#endif // USE_WRAP
