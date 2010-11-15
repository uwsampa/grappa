#include <pthread.h>
#include <stdint.h>
#include <iostream>

const uint32_t MAX = 1000;
uint32_t myarray[MAX];

inline void XBEGIN() {}
inline void XEND()   {}

inline uint32_t stall(uint32_t n) 
{
    for (uint32_t i = 0; i < 0xff; i++) 
    {
    }
    return n;
}

void* func1(void* v) 
{
    XBEGIN();
    for (uint32_t i = 0; i < MAX; i++) 
    {
        myarray[i] = stall(1);
    }
    XEND();
}

void* func2(void* v) 
{
    XBEGIN();
    for (int32_t i = MAX-1; i >= 0; i--) 
    {
        myarray[i] = stall(0);
    }
    XEND();
}

int main(int argc, char** argv) 
{
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, func1, NULL);
    pthread_create(&thread2, NULL, func2, NULL);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    for (int i = 0; i < MAX; i++) 
    {
        std::cout << myarray[i] << " " << std::flush;
        if ((i % 10) == 9) 
        {
            std::cout << "\n" << std::flush;
        }
    }
}

    
