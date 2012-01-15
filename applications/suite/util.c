//
//  util.c
//  AppSuite
//
//  Created by Brandon Holt on 1/14/12.
//  Copyright (c) 2012 University of Washington. All rights reserved.
//

#include <stdio.h>
#include "defs.h"

void printArray(char * title, graphint * array, graphint size) {
	printf("%s[", title);
	if (size > 0) printf("%d", array[0]);
	for (int i = 1; i < size; i++) printf(", %d", array[i]);
	printf("]\n");
}
