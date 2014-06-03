#pragma once

namespace Aggregates {
  template < typename State, typename UV >
    State SUM(State sofar, UV nextval) {
      return sofar + nextval;
    }

  template < typename State, typename UV >
    State COUNT(State sofar, UV nextval) {
      return sofar + 1;
    }
}
