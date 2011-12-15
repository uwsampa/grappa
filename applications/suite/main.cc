//
//  main.c
//  AppSuite
//
//  Created by Brandon Holt on 12/13/11.
//  Copyright 2011 University of Washington. All rights reserved.
//

#include <stdio.h>

#include "defs.h"

int main(int argc, char* argv[]) {
	
	double start = timer();
	
	graphSDG ingraph(4);
	
	graph g(3, 4);
	
	double end = timer();
	
	printf("time = %g s\n", end-start);
	
	return 0;
}
