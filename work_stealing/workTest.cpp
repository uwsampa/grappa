#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>
#include "CircularWSDeque.hpp"



typedef struct work_t {
    int n;
    int originator;
} work_t;

template class CircularArray<work_t>;
template class CircularWSDeque<work_t>;

#define NUM_THREADS 2
CircularWSDeque<work_t> wsqs[NUM_THREADS];


void printNumbers(work_t w, int tid);
void spawn(int n);

void printNumbers(work_t w, int tid) {
    printf("thread %d: print (n:%d, orig:%d)\n", tid, w.n, w.originator);

    if (w.n >= 2) {
        spawn(w.n-1);
        spawn(w.n-2);
    }
}

void spawn(int n) {
   int tn = omp_get_thread_num();
   
   work_t w;
   w.n = n;
   w.originator = tn;
   
    wsqs[tn].pushBottom(w);
}

bool getWork(work_t* res) {
   const int tn = omp_get_thread_num();

   work_t ele;
   if (CWSD_OK == wsqs[tn].popBottom(&ele)) {
       *res = ele;
       return true;
   } else {
       while (1) {
           int i = random() % NUM_THREADS;
           if (i==tn) continue;

           if (CWSD_OK == wsqs[i].steal(&ele)) {
                *res = ele;
                printf("steal\n");
                return true;
           } else {
               return false;
           }
       }
   }
}



int main() {
    
    #pragma omp parallel for num_threads(NUM_THREADS)
    for (int i=0; i<NUM_THREADS; i++) {
        const int tn = omp_get_thread_num();
        work_t initialWork;
        initialWork.n = 6;
        initialWork.originator = tn;
        wsqs[tn].pushBottom(initialWork);

        work_t work;
        while (getWork(&work)) {
            printNumbers(work, tn);
            if (tn==1) sleep(1); // force an opportunity for some stealing
        }
    }
}

    

