#pragma once

#include <cstdint>
#include <unordered_set>
#include <utility>
#include <functional>


typedef std::pair<int64_t,int64_t> pair_t;
namespace std {
  template <> struct hash<pair_t> {
    size_t operator()(const pair_t& x) const {
      static int64_t p = 32416152883; // prime
      return p*x.first + x.second; 
    }
  };
}



int64_t fourth_root(int64_t x);

std::function<int64_t (int64_t)> makeHash( int64_t dim );
