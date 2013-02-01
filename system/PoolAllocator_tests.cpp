#include <Grappa.hpp>
#include <Message.hpp>
#include <MessagePool.hpp>
#include <ForkJoin.hpp>

#include <boost/test/unit_test.hpp>

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( PoolAllocator_tests );

void user_main(void* ignore) {
  LocalTaskJoiner joiner;
  
  int x[10];

  {
    MessagePool<2048> pool;
    
    for (int i=0; i<10; i++) {
      joiner.registerTask();
      auto* f = pool.message(1, [i,&x,&joiner]{
        VLOG(1) << "I'm message " << i << "!";
        send_heap_message(0, [i,&x, &joiner]{
          VLOG(1) << "Message " << i << " signalling";
          x[i] = i;
          joiner.signal();
        });
      });
      f->send();
    }
  }
  joiner.wait();
  
  for (int i=0; i<10; i++) {
    BOOST_CHECK_EQUAL(i, x[i]);
  }
  VLOG(1) << "done";
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
