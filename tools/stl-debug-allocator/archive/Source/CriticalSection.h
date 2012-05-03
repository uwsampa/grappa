#ifndef CRITICAL_SECTION_H_
#define CRITICAL_SECTION_H_ 1
/*
 * $Id: CriticalSection.h,v 1.1 2001/10/21 03:19:57 root Exp root $
 *
 * Support for synchronizing threads on a resource
 * in a multi-thread environment.
 */

#ifdef WIN32
 // Make sure <windows.h> is included, for CRITICAL_SECTION
 #ifndef STRICT
  #define STRICT
 #endif
 #include <windows.h>
#endif

#ifdef UNIX
 #include <pthread.h>
#endif

#if !defined(_MT)
class CriticalSection
{ /* empty */ };

class Lock
{
public:
    explicit Lock(const CriticalSection& cs) {}
    ~Lock() {}
};

#else
 ///////////////////////////////////////////////////////////////
 // multithreaded application model

class CriticalSection
{
private:
    // Disable copy constructor and 
    // assignment operator. Critical 
    // sections cannot be copied.
    CriticalSection(const CriticalSection&);

    CriticalSection& operator=(
        const CriticalSection&);

    //----------------------------------------------------------
    //
#if defined(UNIX)
    
public:
    CriticalSection()
    { ::pthread_mutex_init(&mutex_, 0); }

    ~CriticalSection()
    { ::pthread_mutex_destroy(&mutex_); }

    void Enter() const
    { ::pthread_mutex_lock(&mutex_); }

    void Leave() const
    { ::pthread_mutex_unlock(&mutex_); }

private:
    mutable pthread_mutex_t mutex_;

    //----------------------------------------------------------
    //
#elif defined(WIN32)

public:
    CriticalSection()
    { ::InitializeCriticalSection(&cs_); }

    ~CriticalSection()
    { ::DeleteCriticalSection(&cs_); }

    void Enter() const 
    { ::EnterCriticalSection(&cs_); }

    void Leave() const
    { ::LeaveCriticalSection(&cs_); }

private:
    mutable CRITICAL_SECTION cs_;
#endif // WIN32
};

#ifdef UNIX
inline pthread_t GetCurrentThreadId()
{ return pthread_self(); }
#endif
    
/**
 * Locks a critical section, automatically
 * releases it when destroyed
 */
class Lock
{
public:
    explicit Lock(const CriticalSection& cs) : cs_(cs)
    {
    #if _DEBUG
        std::clog << "Lock::Lock(" << &cs << ") thread=";
        std::clog << GetCurrentThreadId() << std::endl;
    #endif
        cs_.Enter();
    }

    ~Lock()
    {
        cs_.Leave();
    #if _DEBUG
        std::clog << "Lock::~Lock() thread=";
        std::clog << GetCurrentThreadId() << std::endl;
    #endif
    }

private:
    Lock(const Lock&);
    Lock& operator=(const Lock&);

private:
    const CriticalSection& cs_;
};
#endif // _MT -- multithreaded model
#endif // CRITICAL_SECTION_H_
