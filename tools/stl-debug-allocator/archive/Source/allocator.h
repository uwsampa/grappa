/*
 * $Id: allocator.h,v 1.1 2001/10/21 03:19:57 root Exp root $
 */
#ifndef ALLOCATOR_
#define ALLOCATOR_ 1

#include <new>
#include <cstddef>

// Distinguish between Microsoft compiler
// and CodeWarrior; CW defines _MSC_VER
// (for compatibility with MFC? ATL?)

#if defined(_MSC_VER) && !defined(__MWERKS__)
 #define MICROSOFT_COMPILER 1
#endif

namespace STLMemDebug
{
#if defined(__MWERKS__)
    // typedefs for CodeWarrior 6.0
    typedef std::size_t size_t;
    typedef std::ptrdiff_t ptrdiff_t;    
#endif

    // Returns the maximum value 
    // that can be represented as
    // a size_t
    size_t size_t_max();
    
    // Memory access modes
    enum AccessMode
    { 
        memNoAccess, 
        memReadOnly, 
        memReadWrite 
    };
        
    
    // Allocates n bytes of raw memory,
    // can throw std::bad_alloc
    void* allocate(size_t n);
        
    void deallocate(void* p, size_t n) throw();

    // Calculates the amount of memory 
    // needed to store n objects of given 
    // size. Some rounding up may occur if
    // necessary.
    size_t calcMemSize(size_t n, size_t size);

    /**
     * Abstract interface for a memory
     * manager class that keeps track of
     * the allocated memory blocks.
     * Memory blocks are described by 
     * their start addresses and their sizes.
     */
    class MemMgr
    {
    public:
        virtual ~MemMgr() {}

        // Sets access mode for 
        // all managed memory blocks
        virtual void setAccessMode(AccessMode) = 0;

        // Inserts block info into the 
        // internal list of mem blocks
        virtual void insert(const void*, size_t) = 0;

        // Remove memory block info
        virtual void erase(const void*, size_t) = 0;

        // Returns the number of managed blocks
        virtual size_t size() const = 0;

        // Prints allocated memory 
        // block info to log console,
        // for debugging purposes.
        virtual void dumpBlocks() const = 0;
        
        // Returns true if there is a
        // mem block starting at the given address.
        virtual bool search(const void* addr) const = 0;
    };

    /**
     * Base class for our allocator. Provides
     * access to the Memory Manager class.
     */
    class BaseAllocator
    {       
    public:
        /**
         * All instantiations of the allocator share the same base
         * class and thus use the same Memory Manager to keep track
         * of the allocated blocks.
         */
        static MemMgr& getMemMgr();

        /**
         * A helper class that uses RAII
         * (resource-acquisition-is-initialization)
         * to make all memory blocks allocated with
         * the BaseAllocator read-only.
         */
        struct MemReadOnlyScope
        {   
            MemReadOnlyScope()
            { getMemMgr().setAccessMode(memReadOnly); }

            ~MemReadOnlyScope()
            { getMemMgr().setAccessMode(memReadWrite); }
        };

        /* /\** */
        /*  * A helper class that uses RAII */
        /*  * (resource-acquisition-is-initialization) */
        /*  * to make all memory blocks allocated with */
        /*  * the BaseAllocator read-only. */
        /*  *\/ */
        /* struct MemScope */
        /* { */
        /*     MemScope( ) */
        /*     { getMemMgr().setAccessMode(memRead); } */

        /*     ~MemScope() */
        /*     { getMemMgr().setAccessMode(memReadWrite); } */
        /* }; */

        /**
         * Allocate nbytes of raw memory, can throw
         * std::bad_alloc
         */
        static void* allocate(size_t nbytes)            
        {
            void* ptr = STLMemDebug::allocate(nbytes);
            
            // register the newly allocated block
            // with the memory manager object
            getMemMgr().insert(ptr, nbytes);
            return ptr;
        }

        /**
         * Free the memory allocated by the method above.
         */
        static void deallocate(void* ptr, size_t nbytes)
            throw()
        {
            // remove memory block starting at
            // 'p' and of size 'size' from
            // the memory manager's internal
            // list of allocated block

            getMemMgr().erase(ptr, nbytes);
            STLMemDebug::deallocate(ptr, nbytes);      
        }

    private:
        static MemMgr* pMgr_;
    };

    /**
     * Allocator for debugging memory 
     * corruption bugs with STL containers. 
     * It provides methods for controlling the
     * access mode for the allocated memory.
     */
    template<class T>
    class Allocator : public BaseAllocator
    {
    public:
        typedef size_t     size_type;
        typedef ptrdiff_t  difference_type;
        typedef T*         pointer;
        typedef const T*   const_pointer;
        typedef T&         reference;
        typedef const T&   const_reference;
        typedef T          value_type;

        // Scott Meyers' Effective STL (Item 10, pag 53)
        // explains why the rebind struct is needed.
        template <class U>
        struct rebind
        {
            typedef Allocator<U> other;
        };

        // Constructors: empty, no state
        Allocator() throw() {}

        template<class U> Allocator(
            const Allocator<U>&) throw() {}

        Allocator(const Allocator&) throw() {}
        ~Allocator() throw() {}

        // Return the maximum number of elements
        // that can be allocated
        size_type max_size() const throw()
        { return size_t_max() / sizeof(T); }

        pointer address(reference x) const
        { return &x; }

        const_pointer address(
            const_reference x) const
        { return &x; }

        // Allocate (without initializing) n
        // elements of type T
        pointer allocate(size_type n, const void* /* hint */ = 0)
        {
            const size_type size =
                calcMemSize(n, sizeof(T));

            return reinterpret_cast<pointer>(
                BaseAllocator::allocate(size));
        }
        
#if (MICROSOFT_COMPILER)
        // Allocate n characters, non-standard
        // behavior required by Dinkumware STL
        // (shipped with MSVC 6.0)
        char* _Charalloc(size_type n)
        {
            const size_type size = calcMemSize(n, 1);
            return reinterpret_cast<char*>(
                BaseAllocator::allocate(size));
        }

        // Work around bug in Dinkumware STL,
        // looks like 'allocator' is used where
        // a rebind.other would've been required?

        // Also, is there a way to detect
        // the Dinkumware library?

        template<class U>
        void deallocate(U* p, size_type n)
        {
            const size_type size =
                calcMemSize(n, sizeof(U));
            
            BaseAllocator::deallocate(p, size);
        }
#else       
        // Deallocate n elements at address p
        void deallocate(pointer p, size_type n)
        {
            const size_type size =
                calcMemSize(n, sizeof(T));               
            BaseAllocator::deallocate(p, size);
        }
#endif // MICROSOFT_COMPILER


#if defined(__MWERKS__)
        // Work around CodeWarrior odd behavior
                
        // Initialize element at address p with value
        template<class U>
        void construct(U* p, const U& value)
        { new(p) U(value); }

        // Destroy elements of initialized storage p
        template<class U>
        void destroy(U* p)
        { p->~U(); }
#else
        void construct(pointer p, const T& value)
        { new(p) T(value); }
        
        void destroy(pointer p)
        { p->~T(); }
        
#endif // ! CodeWarrior
    }; // class Allocator


    // Return that all specializations of 
    // this Allocator are interchangeable
    template<class T1, class T2>
    bool operator==(
        const Allocator<T1>&, 
        const Allocator<T2>&) throw()
    { return true; }

    template<class T1, class T2>
    bool operator!=(
        const Allocator<T1>&, 
        const Allocator<T2>&) throw()
    { return false; }   
} // namespace STLMemDebug

#endif // ALLOCATOR_
