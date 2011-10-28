#include <stdio.h>
#include <omp.h>

int main(int argc, const char * argv[]) {
	
	#pragma omp parallel
	{
		printf("Hello world from thread %d/%d!\n", omp_get_thread_num(), omp_get_num_threads());
	}
	
	return 0;
}