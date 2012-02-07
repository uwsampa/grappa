
#ifndef __DELEGATE_HPP__
#define __DELEGATE_HPP__


void SoftXMT_delegate_write_word( int64_t * address, int64_t data );

int64_t SoftXMT_delegate_read_word( int64_t * address );

int64_t SoftXMT_delegate_fetch_and_add_word( int64_t * address, int64_t data );

#endif
