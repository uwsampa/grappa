/*NO LEGAL*/
#define _WIN32_WINNT 0x0400 // Required to use SignalObjectAndWait()
#include <windows.h>
#include <iostream>
#include <string>

static void Breakpoint();
static DWORD WINAPI Child(LPVOID);
static void Parent();
static DWORD WINAPI NotTheWaiter(LPVOID);
static void TheWaiter();
static DWORD WINAPI TerminatorSyscall(LPVOID);
static void WaiterSyscall();
static DWORD WINAPI TerminatorSpin(LPVOID);
static void WaiterSpin();

static HANDLE WaiterReady;
static HANDLE WaiterObject;
static volatile int Ready = 0;


int main(int argc, char **argv)
{
    std::string arg;
    if (argc == 2)
        arg.assign(argv[1]);

    if (arg == "simple")
    {
        HANDLE h = CreateThread(0, 0, Child, 0, 0, 0);
        if (!h)
        {
            std::cerr << "Unable to create thread\n";
            return 1;
        }
        Parent();

        WaitForSingleObject(h, INFINITE);
        return 0;
    }
    if (arg == "block-syscall")
    {
        WaiterReady = CreateEvent(0, TRUE, FALSE, 0);
        WaiterObject = CreateEvent(0, TRUE, FALSE, 0);

        HANDLE h = CreateThread(0, 0, NotTheWaiter, 0, 0, 0);
        if (!h)
        {
            std::cerr << "Unable to create thread\n";
            return 1;
        }
        TheWaiter();

        WaitForSingleObject(h, INFINITE);
    }
    else if (arg == "terminate-syscall")
    {
        WaiterReady = CreateEvent(0, TRUE, FALSE, 0);
        WaiterObject = CreateEvent(0, TRUE, FALSE, 0);

        HANDLE h = CreateThread(0, 0, TerminatorSyscall, 0, 0, 0);
        if (!h)
        {
            std::cerr << "Unable to create thread\n";
            return 1;
        }
        WaiterSyscall();
    }
    else if (arg == "terminate-spin")
    {
        HANDLE h = CreateThread(0, 0, TerminatorSpin, 0, 0, 0);
        if (!h)
        {
            std::cerr << "Unable to create thread\n";
            return 1;
        }
        WaiterSpin();
    }
    else
    {
        std::cerr << "Invalid argument\n";
        return 1;
    }
    return 0;
}

static void Breakpoint()
{
    /* nothing to do */
}


/* simple thread test */

static DWORD WINAPI Child(LPVOID)
{
    std::cout << "This is the child\n";
    return 0;
}

static void Parent()
{
    std::cout << "This is the parent\n";
}


/* test "cont" and "kill" while blocked in a syscall */

static DWORD WINAPI NotTheWaiter(LPVOID)
{
    WaitForSingleObject(WaiterReady, INFINITE);

    // We set a breakpoint on the following line, at which point
    // we know that the other thread is blocked in a system call.
    //
    Breakpoint();
    SetEvent(WaiterObject);
    return 0;
}

static void TheWaiter()
{
    SignalObjectAndWait(WaiterReady, WaiterObject, INFINITE, FALSE);
}


/* test TerminateProcess() while blocked in a syscall */

static DWORD WINAPI TerminatorSyscall(LPVOID)
{
    WaitForSingleObject(WaiterReady, INFINITE);
    TerminateProcess(GetCurrentProcess(), 0);
    return 0;
}

static void WaiterSyscall()
{
    SignalObjectAndWait(WaiterReady, WaiterObject, INFINITE, FALSE);
}


/* test TerminateProcess() while in a spin loop */

static DWORD WINAPI TerminatorSpin(LPVOID)
{
    while (!Ready)
        Sleep(1);
    TerminateProcess(GetCurrentProcess(), 0);
    return 0;
}

static void WaiterSpin()
{
    Ready = 1;
    for (;;);
}
