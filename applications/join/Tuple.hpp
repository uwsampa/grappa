#ifndef TUPLE_HPP
#define TUPLE_HPP

#include <stdint.h>

#define TUPLE_LEN 2
struct Tuple {
  int64_t columns[TUPLE_LEN];
};

#endif // TUPLE_HPP

