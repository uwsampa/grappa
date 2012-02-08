
#ifndef __BOOST_HELPERS__
#define __BOOST_HELPERS__

#include <boost/test/unit_test.hpp>

#define BOOST_REQUIRE_EQUAL_MESSAGE( L, R, M )				\
  BOOST_CHECK_WITH_ARGS_IMPL( ::boost::test_tools::tt_detail::equal_impl_frwd(), "HELLO", REQUIRE, CHECK_EQUAL, (L)(R) )

#endif
