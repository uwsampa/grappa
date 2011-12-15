//
//  main.c
//  AppSuite
//
//  Created by Brandon Holt on 12/13/11.
//  Copyright 2011 University of Washington. All rights reserved.
//

#include <stdio.h>

#include "timer.h"

int main(int argc, char* argv[]) {
	
	double start = timer();
	
	double end = timer();
	
	printf("time = %g s\n", end-start);
	
	return 0;
}
