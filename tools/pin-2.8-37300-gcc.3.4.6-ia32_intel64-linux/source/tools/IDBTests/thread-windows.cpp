/*NO LEGAL*/
#include <iostream>
#include <cstdlib>
#include <windows.h>

static DWORD WINAPI child(LPVOID);
static void parent();


int main()
{
    HANDLE h = CreateThread(0, 0, child, 0, 0, 0);
    if (!h)
    {
        std::cerr << "Unable to create thread\n";
        return 1;
    }
    parent();

    WaitForSingleObject(h, INFINITE);
    return 0;
}

static DWORD WINAPI child(LPVOID)
{
    malloc(1);
    std::cout << "This is the child\n";
    return 0;
}

static void parent()
{
    malloc(1);
    std::cout << "This is the parent\n";
}
