#pragma once
#include <algorithm>

namespace Aggregates {
  template < typename T >
  class Count {
    private:
      const uint64_t _count;
      Count(uint64_t count) : _count(count) {} 
    public:
      Count init() {
        return Count(0);
      }

      Count add(const T& val) {
        return Count(_count+1);
      }

      Count combine(const Count& other) {
        return Count(_count+other._count);
      }

      uint64_t emit() {
        return _count;
      }
  };
      
  template < typename T >
  class Avg {
    private:
      const T _sum;
      const uint64_t _count;
      Avg(T sum, uint64_t count) : _sum(sum), _count(count) {} 
    public:
      Avg init() {
        return Avg(0, 0);
      }

      Avg add(const T& val) {
        return Avg(_sum+val, _count+1);
      }

      Avg combine(const Avg& other) {
        return Avg(_sum+other._sum, _count+other._count);
      }

      double emit() {
        return static_cast<double>(_sum) / _count;
      }
  };
      

  template < typename T >
  class Sum { 
    private:
    const T _state;
    Sum(T state) : _state(state) {}

    public:
    Sum init() {
      return Sum(0);
    }
    
    Sum add(const T& val) {
      return Sum(_state+val);
    }

    Sum combine(const Sum& other) {
      return Sum(_state+other._state);
    }

    Sum emit() {
      return _state;
    }
  };

  template < typename State, typename UV >
    State SUM(const State& sofar, const UV& nextval) {
      return sofar + nextval;
    }

  template < typename State, typename UV >
    State COUNT(const State& sofar, const UV& nextval) {
      return sofar + 1;
    }

  // keep MAX macro from being used here
#pragma push_macro("MAX")
#undef MAX
  template <typename State, typename UV >
    State MAX(const State& sofar, const UV& nextval) {
      return std::max(sofar, nextval); 
    }
#pragma pop_macro("MAX")
  // keep MIN macro from being used here
#pragma push_macro("MIN")
#undef MIN
  template <typename State, typename UV >
    State MIN(const State& sofar, const UV& nextval) {
      return std::min(sofar, nextval); 
    }
#pragma pop_macro("MIN")


  template <typename N>
    N Zero() {
      return 0;
    }

}
