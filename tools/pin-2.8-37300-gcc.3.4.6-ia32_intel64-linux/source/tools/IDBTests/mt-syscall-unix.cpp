/*NO LEGAL*/
#include <iostream>
#include <pthread.h>
#include <time.h>

static void *NotTheWaiter(void *);
static void TheWaiter();
static void Breakpoint();


int main()
{
    pthread_t tid;
    if (pthread_create(&tid, 0, NotTheWaiter, 0) != 0)
    {
        std::cerr << "Unable to create thread\n";
        return 1;
    }
    TheWaiter();

    pthread_join(tid, 0);
    return 0;
}


static void *NotTheWaiter(void *)
{
    // Wait a little while in hopes that TheWaiter() will reach its system call.
    // The test will still pass even if it doesn't get to the sysem call.
    //
    struct timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    nanosleep(&ts, 0);

    Breakpoint();
    return 0;
}

static void TheWaiter()
{
    struct timespec ts;
    ts.tv_sec = 5;
    ts.tv_nsec = 0;
    nanosleep(&ts, 0);
}

static void Breakpoint()
{
}
