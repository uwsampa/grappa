#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define START 0.0
#define END 1.0

double func(double x) {
	return x*x;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Wrong number of args.\n");
		return 0;
	}
	long NUM_STEPS = atol(argv[1]);

	double total, width;
	long i;
	width = ((double)(END - START))/NUM_STEPS;

	double start_time, end_time;
	start_time = omp_get_wtime();

	#pragma omp parallel for reduction(+:total)
	for (i = 0; i<NUM_STEPS; i++) {	
		double x = i * width;
		double y = func(x);
		total += y * width;
	}
	end_time = omp_get_wtime();
	
	printf("area under x^2 from 0 to 1 = %g\n%g seconds\n", total, end_time - start_time);
	
	return 0;
}

