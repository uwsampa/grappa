#if HAVE_CONFIG_H
#   include "config.h"
#endif

#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#if HAVE_STDLIB_H
#   include <stdlib.h>
#endif
#if HAVE_STRING_H
#   include <string.h>
#endif
#if HAVE_ASSERT_H
#   include <assert.h>
#endif
#include "iterator.h"

int main(int argc, char **argv) {
  stride_info_t sitr,ditr;
  int a[10][10], b[11][11];
  int asr[1] = {10*sizeof(int)};
  int bsr[1]={11*sizeof(int)};
  int count[2] = {5*sizeof(int),5};
  int i, j;

  for(i=0; i<10; i++) {
    for(j=0; j<10; j++) {
      a[i][j] = i*10+j;
    }
  }
  for(i=0; i<11; i++) {
    for(j=0; j<11; j++) {
      b[i][j] = 0;
    }
  }

  armci_stride_info_init(&sitr, &a[2][3],1,asr,count);
  armci_stride_info_init(&ditr, &b[3][4],1,bsr,count);

  assert(armci_stride_info_size(&sitr) == 5);
  assert(armci_stride_info_size(&ditr) == 5);
  assert(armci_stride_info_pos(&sitr) == 0);
  assert(armci_stride_info_pos(&ditr) == 0);

  while(armci_stride_info_has_more(&sitr)) {
    int bytes;
    char *ap, *bp;
    assert(armci_stride_info_has_more(&ditr));

    bytes = armci_stride_info_seg_size(&sitr);
    assert(bytes == armci_stride_info_seg_size(&ditr));
    
    ap = armci_stride_info_seg_ptr(&sitr);
    bp = armci_stride_info_seg_ptr(&ditr);

    memcpy(bp,ap,bytes);

    armci_stride_info_next(&sitr);
    armci_stride_info_next(&ditr);
  }
  armci_stride_info_destroy(&sitr);
  armci_stride_info_destroy(&ditr);

#if 0
  for(i=0; i<10; i++) {
    for(j=0; j<10; j++) {
      printf("%3d  ",a[i][j]);
    }
    printf("\n");
  }

  for(i=0; i<11; i++) {
    for(j=0; j<11; j++) {
      printf("%3d  ",b[i][j]);
    }
    printf("\n");
  }
#endif
  
  for(i=2; i<2+5; i++) {
    for(j=3; j<3+5; j++) {
      if(a[i][j] != b[i+1][j+1]) {
	printf("a[%d][%d]=%d b[%d][%d]=%d\n",i,j,a[i][j],i,j,b[i][j]);
	printf("Test Failed\n");
	return 0;
      }
    }
  }
  printf("Success\n");
  return 0;
}

