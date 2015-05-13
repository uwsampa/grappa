#pragma once
#include <algorithm>

namespace Aggregates {
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
