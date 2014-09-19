#pragma once

namespace Aggregates {
  template < typename State, typename UV >
    State SUM(const State& sofar, const UV& nextval) {
      return sofar + nextval;
    }

  template < typename State, typename UV >
    State COUNT(const State& sofar, const UV& nextval) {
      return sofar + 1;
    }

  int64_t Zero() {
    return 0;
  }
}
