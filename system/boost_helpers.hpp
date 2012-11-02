
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __BOOST_HELPERS__
#define __BOOST_HELPERS__

#include <boost/test/unit_test.hpp>

#define BOOST_REQUIRE_EQUAL_MESSAGE( L, R, M )				\
  BOOST_CHECK_WITH_ARGS_IMPL( ::boost::test_tools::tt_detail::equal_impl_frwd(), "HELLO", REQUIRE, CHECK_EQUAL, (L)(R) )

#endif
