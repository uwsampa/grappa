#include <boost/test/unit_test.hpp>
#include "local_graph.hpp"


BOOST_AUTO_TEST_SUITE( Local_graph_tests );



BOOST_AUTO_TEST_CASE( testBasicList ) {
  BOOST_MESSAGE("Testing basic adj list"); 

  std::vector<Edge> edges;
  edges.push_back({4,5});
  edges.push_back({6,7});
  edges.push_back({10,11});

  LocalAdjListGraph g(edges);
  BOOST_CHECK( g.neighbors(4)[0] == 5 );
  BOOST_CHECK( g.neighbors(6)[0] == 7 );
  BOOST_CHECK( g.neighbors(10)[0] == 11 );
}

BOOST_AUTO_TEST_SUITE_END();
