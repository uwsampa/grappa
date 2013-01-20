
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include <Grappa.hpp>
#include <Message.hpp>
#include <FullEmpty.hpp>
#include <Message.hpp>

BOOST_AUTO_TEST_SUITE( New_delegate_tests );

using namespace Grappa;

int64_t delegate_read(GlobalAddress<int64_t> target) {
  FullEmpty<int64_t> result;
  Node origin = Grappa_mynode();

  send_message(target.node(), [=]{
    CHECK(target.node() == Grappa_mynode());
    int64_t val = *target.pointer();
    std::cout << "val = " << val << "\n";

    send_message(origin, [=]{
      std::cout << "val = " << val << " (back on origin)\n";
      result.writeEF(val);
    });
  });

  return result.readFE();
}

void user_main(void * args) {
  bool done = false;
 
  int64_t seed = 12345;
  GlobalAddress<int64_t> seed_addr = make_global(&seed);

  send_message(1, [=]{
    // on node 1
    privateTask([=]{
      int64_t vseed = delegate_read(seed_addr);

      std::cout << "delegate_read(seed) = " << vseed << "\n";
    });
  }]);


  std::cout << "d = " << d.readFE() << std::endl;
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
	       &(boost::unit_test::framework::master_test_suite().argv)
	       );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
