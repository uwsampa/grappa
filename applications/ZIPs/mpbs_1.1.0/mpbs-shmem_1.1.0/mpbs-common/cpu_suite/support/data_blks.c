/* CVS info                         */
/* $Date: 2010/04/01 21:48:02 $     */
/* $Revision: 1.5 $                 */
/* $RCSFile$                        */
/* $State: Stab $:                   */

#include <stdio.h>
#include <string.h>
#include "data_blks.h"

unsigned long data_block_1024(char* data){
  size_t j = 0;
  unsigned long sum = 0;

  // Write the alphabet on each line and checksum
  // in an endian neutral way
  while(1){
     int k = 0;
     while(k<26){
        data[j] = (char)((int)'a'+k);
        sum += (unsigned long)data[j] << ((j%8)*8);
        j++;
	if (j == 1023) break;
        k++;
     }
     data[j] = '\n';
     sum += (unsigned long)data[j] << ((j%8)*8);
     j++;
     if (j>=1024) break;
  }

  return sum;
}

void fill_buffer(char* data, size_t blocks){
  size_t i;

  for(i=1; i<blocks/2; i*=2){
    memcpy(data+i*1024, data, i*1024);
  }
  i =  blocks - i;
  memcpy(data+i*1024, data, i*1024);
}

size_t cksm_buffer(char* data, size_t blocks, unsigned long gsum ){
  size_t i, j, k;
  unsigned long sum;

  for(j=0; j<blocks*1024; j+=1024){
    sum = 0;
    for(i=0; i<1024; i++){
      sum += (unsigned long)data[j+i] << (i%8)*8;
    }
    if ( sum != gsum ){
      return j;
    }
  }
  return __BM_GOOD_BLK;
}



/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make .o name"
End:
*/



