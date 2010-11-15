/*NO LEGAL*/
#ifdef TARGET_WINDOWS
#   include <windows.h>
#else
#   include <time.h>
#endif

void (*volatile Before)();
void (*volatile After)();


extern "C" void BeforeSleep()
{
}

extern "C" void AfterSleep()
{
}

int main()
{
    Before = BeforeSleep;
    After = AfterSleep;

#ifdef TARGET_WINDOWS
    Before();
    Sleep(5000);
    After();
#else
    // Call nanosleep() once here to cause the dynamic linker to resolve the symbol.
    // This reduces the number of single-steps we need to do between Before() and
    // After().
    //
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    nanosleep(&ts, 0);

    Before();
    ts.tv_sec = 5;
    ts.tv_nsec = 0;
    nanosleep(&ts, 0);
    After();
#endif

    return 0;
}
