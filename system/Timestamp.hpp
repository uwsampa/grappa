
#ifndef __TIMESTAMP_HPP__
#define __TIMESTAMP_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "common.hpp"

typedef uint64_t SoftXMT_Timestamp;
extern SoftXMT_Timestamp SoftXMT_current_timestamp;

static inline SoftXMT_Timestamp SoftXMT_tick() {
  SoftXMT_Timestamp old_timestamp = SoftXMT_current_timestamp; 
  SoftXMT_current_timestamp = rdtsc();
  return old_timestamp;
}
  
static inline SoftXMT_Timestamp SoftXMT_get_timestamp() { 
  //SoftXMT_tick();
  return SoftXMT_current_timestamp;
}

#endif
