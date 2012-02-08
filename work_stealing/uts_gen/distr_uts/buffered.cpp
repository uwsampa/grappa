#include "Delegate.hpp"
#include "buffered.hpp"

<template class T>
T* get_local_copy_of_remote(global_address addr, size_t num_ele) {
    T* buf = (T*) malloc (num_ele * sizeof(T)); // XXX malloc'ing every acquire

    const size_t xfer_size = sizeof(int64_t);
    size_t num_xfers = (num_ele*sizeof(T))/xfer_size;
    for (int i=0; i<num_xfers; i++) { 
        int64_t data;
        data = SoftXMT_delegate_read_word( addr ); //FIXME: will yield num_xfers times
                                                   // eventually want yield separate altogether
        memcpy(((char*)dest)+(i*xfer_size), &data, xfer_size);
         
        addr.offset += xfer_size; //FIXME: assumes all same node (use global_address ADT incr)
    }

    return buf;
}

<template class T>
void release_local_copy(T* buf) {
    free(buf);
}

