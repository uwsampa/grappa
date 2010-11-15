/*NO LEGAL*/
#include <iostream>
#include <pthread.h>

static void *Child(void *);
static void Parent();


int main()
{
    pthread_t tid;
    if (pthread_create(&tid, 0, Child, 0) != 0)
        std::cerr << "Unable to create thread\n";
    Parent();

    pthread_join(tid, 0);
    return 0;
}


static void *Child(void *)
{
    std::cout << "This is the child\n";
    return 0;
}

static void Parent()
{
    std::cout << "This is the parent\n";
}
