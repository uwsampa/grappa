

#include <stdint.h>
#include <iostream>
#include <cassert>
#include <ctime>
#include <sched.h>


#include "MCRingBuffer.hpp"




void setaffinity(int i) {
  cpu_set_t set;
  CPU_ZERO(&set);
  if (i < 6) {
    CPU_SET(i*2,&set);
  } else {
    CPU_SET((i-6)*2+1,&set);
  }
  sched_setaffinity(0, sizeof(cpu_set_t), &set);
  CPU_ZERO(&set);
  sched_getaffinity(0, sizeof(cpu_set_t), &set);
}



int main() {
  {
    struct timespec start, end;
    uint64_t iters = 1 << 26;
    uint64_t sum = 0;
    volatile bool go = false;
    //MCRingBuffer<> q;

    MCRingBuffer<uint64_t, 8, 32, 64> q;
    //MCRingBuffer<uint64_t, 21, (1<<14), 64> q;

    go = true;
    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel sections
    {
      
#pragma omp section
      {
	setaffinity(0);
	while (!go);
	for(uint64_t i=1; i < iters; ++i) {
	  //std::cout << "sent " << i << std::endl;
	  while (!q.produce(i));
	}
	q.flush();
      }

#pragma omp section
      {
	setaffinity(1);
	for(uint64_t i=1; i < iters; ++i) {
	  uint64_t val = 0;
	  while (!q.consume(&val));
	  sum += val;
	  //std::cout << "got " << val << std::endl;
	}
      }

    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    std::cout << "Queue sum: " << sum << std::endl;
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
    double throughput = (double)(iters-1) / ( (double)runtime_ns / 1000000000.0 );

    std::cout << "Queue throughput: " << throughput << " elements/s, " << throughput*sizeof(uint64_t) << " B/s" << std::endl;

  }


  return 0;
}
