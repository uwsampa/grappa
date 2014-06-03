#pragma once

template < typename V >
V SUM(V sofar, V nextval) {
  return sofar + nextval;
}

template < typename NUMERIC, typename UV >
NUMERIC COUNT(NUMERIC sofar, UV nextval) {
  return sofar + 1;
}
