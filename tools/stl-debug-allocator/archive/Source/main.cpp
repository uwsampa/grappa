/*
 * $Id: main.cpp,v 1.1 2001/10/21 03:19:57 root Exp root $
 * Test for Allocator
 * Linux compile command line:
 * g++ allocator.cpp main.cpp -Wl,--wrap,free,--wrap,mprotect
 */
#include "includes.h"
#include "allocator.h"

#include <map>
#include <string>
#include <csignal>

using namespace std;
using namespace STLMemDebug;

#ifdef NO_STLMEM_DEBUG
 typedef map<int, string> MapType;
#else
 typedef map<int, string, less<int>, 
    Allocator<pair<const int, string> > > MapType;
#endif 

// This function corrupts the memory
void offender(const void* ptr)
{
    const char trash[] = "TRASH THE MEMORY";
    ::memcpy(const_cast<void*>(ptr), trash, sizeof(trash));
}

// Prints map element. Although the code
// is correct, the function may crashes when
// invoked from the main program
void printElem(const MapType& m, int i)
{
    MapType::const_iterator iter(m.find(i));

    if (iter != m.end())
    {
        cout << iter->second << endl;        
    }
    else
    {
        cout << "element not found" << endl;
    }
}

/* void sigHandler(int sigNum)
{
    cout << "signal caught: " << sigNum << endl;
    exit(sigNum);
} */

int main()
{
    MapType m;

    // Initialize the map
    m.insert(MapType::value_type(1, "one"));
    m.insert(MapType::value_type(2, "two"));

    // signal(SIGSEGV, sigHandler);
    {
        // A convenient way to specify a read-only
        // scope. RAII takes care of restoring to
        // read-write
        BaseAllocator::MemReadOnlyScope readOnlyScope;
        
        offender(&(m.find(1)->second));
        printElem(m, 1);
    } 
    return 0;
}
