#include <boost/test/unit_test.hpp>
#include "Hypercube.hpp"


BOOST_AUTO_TEST_SUITE( Hypercube_tests );



BOOST_AUTO_TEST_CASE( test3d_1i ) {
  BOOST_MESSAGE("Testing 3d, 1 iter"); 

  Hypercube hg( {3,3,3} );

  HypercubeSlice& h = hg.slice({HypercubeSlice::ALL, 0, 0});
  int64_t ii = 0;
  for (auto i : h) {
    BOOST_MESSAGE(i << " " << ii);
    BOOST_CHECK(i == ii);
    ii++;
  }
}

BOOST_AUTO_TEST_CASE( test3d_2i ) {
  BOOST_MESSAGE("Testing 3d, 2 iter x y"); 

  Hypercube hg({3,3,3});

  HypercubeSlice& h = hg.slice({HypercubeSlice::ALL, HypercubeSlice::ALL, 1});
  int64_t ii = 9;
  for (auto i : h) {
    BOOST_MESSAGE(i << " " << ii);
    BOOST_CHECK(i == ii);
    ii++;
  }
}

BOOST_AUTO_TEST_CASE( test3d_2i_apart ) {
  BOOST_MESSAGE("Testing 3d, 2 iter x z"); 

  std::vector<int64_t> shares {3,3,3};
  Hypercube hg(shares);

  HypercubeSlice& h = hg.slice({HypercubeSlice::ALL, 1, HypercubeSlice::ALL});
  int64_t ii = 0;

  int64_t expected[9] = {3,4,5,  
                         12,13,14,
                         21,22,23};

  for (auto i : h) {
    BOOST_MESSAGE(i << " " << expected[ii]);
    BOOST_CHECK(expected[ii] == i);
    ii++;
  }
}

BOOST_AUTO_TEST_CASE( test4d ) {
  BOOST_MESSAGE("Testing 4d"); 

  std::vector<int64_t> shares {2,2,2,2};
  Hypercube hg(shares);

  HypercubeSlice& h = hg.slice({0,0,HypercubeSlice::ALL, HypercubeSlice::ALL});
  int64_t ii = 0;

  int64_t expected[4] = {0, 4, 8, 12};  

  for (auto i : h) {
    BOOST_MESSAGE(i << " " << expected[ii]);
    BOOST_CHECK(expected[ii] == i);
    ii++;

    BOOST_CHECK(ii < 5);
  }
}

BOOST_AUTO_TEST_SUITE_END();
