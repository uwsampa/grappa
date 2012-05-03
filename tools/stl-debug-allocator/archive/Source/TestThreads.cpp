/*
 *  $Id: TestThreads.cpp,v 1.1 2001/10/21 03:19:57 root Exp root $
 *
 *  Test the allocator in a multi-thread environment
 */
#include "includes.h"
#include "allocator.h"
#include <map>
#include <string>
#include <time.h>

#ifdef UNIX
 typedef void* thread_result_t;
 #define __stdcall
#elif defined(WIN32)

 typedef unsigned long thread_result_t;
#endif 

#ifndef _MT
#error _MT not defined
#endif 

typedef void* thread_param_t;

using namespace std;

typedef map<int, string, less<int>,
    STLMemDebug::Allocator<pair<int, string> > > MapType;

/**
 * Thread entry point
 */
thread_result_t __stdcall
ThreadProc(thread_param_t nano_sec)
{
#ifdef UNIX
    timespec delay = { 0, reinterpret_cast<long>(nano_sec) };
#endif
    MapType m;

    for (size_t i(0); i < 10; ++i)
    {
        m.insert(MapType::value_type(i, "some string"));
        
#ifdef UNIX
        nanosleep(&delay, 0);
#else
        // TODO: delay
#endif
    }
    cout << "thread " << ::GetCurrentThreadId() << "done." << endl;
    return 0;
}

/**
 * start a new thread, with given delay
 */
void SpawnThread(long delay)
{
#ifdef UNIX
    pthread_t t = 0;
    pthread_create(&t, 0, ThreadProc, reinterpret_cast<void*>(delay));
#else
    DWORD id(0);

    HANDLE t(::CreateThread(0, 0, 
        ThreadProc, reinterpret_cast<void*>(delay), 0, &id));
    //
    // TODO: Windows
    //
#endif
    clog << "SpawnThread() thread=" << t << endl;
}

int main()
{
    SpawnThread(1000);
    SpawnThread(500);

    clog << flush;
    return 0;
}

