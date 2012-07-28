#ifndef __BM_DBLK_H
#define __BM_DBLK_H

unsigned long data_block_1024(char* data);
void fill_buffer(char* data, size_t blocks);
size_t cksm_buffer(char* data, size_t blocks, unsigned long gsum );

#define __BM_VALID_IO_CHSM     (0xfe030a66fd0460c2L)
#define __BM_GOOD_BLK          (  (size_t) (-1) )

#endif
