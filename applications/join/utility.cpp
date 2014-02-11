#include "utility.hpp"
#include <vector>

int64_t fourth_root(int64_t x) {
  // index pow 4
  std::vector<int64_t> powers = {0, 1, 16, 81, 256, 625, 1296, 2401};
  int64_t ind = powers.size() / 2;
  int64_t hi = powers.size()-1;
  int64_t lo = 0;
  while(true) {
    if (x == powers[ind]) {
      return ind;
    } else if (x > powers[ind]) {
      int64_t next = (ind+hi)/2;
      if (next - ind == 0) {
        return ind;
      }
      lo = ind;
      ind = next;
    } else {
      int64_t next = (ind+lo)/2;
      hi = ind;
      ind = next;
    }
  }
}


std::function<int64_t (int64_t)> makeHash( int64_t dim ) {
  // identity
  return [dim](int64_t x) { return x % dim; };
}

