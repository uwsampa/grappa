#ifndef TUPLE_HPP
#define TUPLE_HPP

#include <stdint.h>
#include <iostream>

#define TUPLE_LEN 2
struct Tuple {
  int64_t columns[TUPLE_LEN];
};

std::ostream& operator<< (std::ostream& o, Tuple& t);

#endif // TUPLE_HPP

