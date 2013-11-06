#include "Tuple.hpp"
#include <sstream>

std::ostream& operator<< (std::ostream& o, Tuple& t) {
  std::stringstream ss;
  ss << "(";
  for ( uint64_t i=0; i<TUPLE_LEN; i++) {
    ss << " " << t.columns[i];
    ss << ",";
  }
  ss << ")";
  o << ss.str();
}
