#include <Grappa.hpp>
#include <Addressing.hpp>
#include <Message.hpp>
#include <MessagePool.hpp>
#include <ForkJoin.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>

#include <boost/test/unit_test.hpp>

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( PoolAllocator_tests );

void test_pool1() {
  LocalTaskJoiner joiner;
  
  int x[10];

  {
    MessagePool<2048> pool;
    
    for (int i=0; i<10; i++) {
      joiner.registerTask();
      auto* f = pool.message(1, [i,&x,&joiner]{
        BOOST_MESSAGE("I'm message " << i << "!");
        send_heap_message(0, [i,&x, &joiner]{
          BOOST_MESSAGE("Message " << i << " signalling");
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
}

void test_pool2() {
  LocalTaskJoiner joiner;
  
  int x[10];
  MessagePool<2048> pool;

  for (int i=0; i<10; i++) {
    joiner.registerTask();
    auto f = pool.send_message(1, [i,&x,&joiner]{
      BOOST_MESSAGE("I'm message " << i << "!");
      send_heap_message(0, [i,&x, &joiner]{
        BOOST_MESSAGE("Message " << i << " signalling");
        x[i] = i;
        joiner.signal();
      });
    });
  }
  pool.block_until_all_sent();

  joiner.wait();
  
  for (int i=0; i<10; i++) {
    BOOST_CHECK_EQUAL(i, x[i]);
  }
}

static int test_async_x;

void test_async_delegate() {
  MessagePool<2048> pool;
  
  delegate::AsyncHandle<bool> a;
  a.call_async(pool, 1, []()->bool {
    test_async_x = 7;
    BOOST_MESSAGE( "x = " << test_async_x );
    return true;
  });
  
  BOOST_CHECK_EQUAL(a.get_result(), true);
  BOOST_CHECK_EQUAL(delegate::read(make_global(&test_async_x, 1)), 7);
}


void user_main(void* ignore) {
  test_pool1();
  // test_pool2();
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
