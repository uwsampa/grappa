#include <Grappa.hpp>
#include <Addressing.hpp>
#include <Message.hpp>
#include <MessagePool.hpp>
#include <ForkJoin.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>

#include <boost/test/unit_test.hpp>

using namespace Grappa;
using Grappa::wait;

BOOST_AUTO_TEST_SUITE( PoolAllocator_tests );

void test_pool1() {
  BOOST_MESSAGE("Test MessagePool");
  LocalTaskJoiner joiner;
  
  int x[10];

  {
    MessagePoolStatic<2048> pool;
    
    for (int i=0; i<10; i++) {
      joiner.registerTask();
      auto* f = pool.message(1, [i,&x,&joiner]{
        send_heap_message(0, [i,&x, &joiner]{
          x[i] = i;
          joiner.signal();
        });
      });
      f->enqueue();
    }
  }
  joiner.wait();
  
  for (int i=0; i<10; i++) {
    BOOST_CHECK_EQUAL(i, x[i]);
  }
}

void test_pool2() {
  BOOST_MESSAGE("Test pool block_until_all_sent");
  LocalTaskJoiner joiner;
  
  int x[10];
  MessagePoolStatic<2048> pool;

  for (int i=0; i<10; i++) {
    joiner.registerTask();
    auto f = pool.send_message(1, [i,&x,&joiner]{
      send_heap_message(0, [i,&x, &joiner]{
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

static int external_test_count = 0;

void test_pool_external() {
  BOOST_MESSAGE("Test MessagePoolExternal");
  
  {
    char buffer[1024];
    MessagePool pool(buffer, 1<<16);
    
    ConditionVariable cv;
    auto cv_addr = make_global(&cv);
    
    pool.send_message(1, [cv_addr]{
      external_test_count = 1;
      signal(cv_addr);
    });
    wait(&cv);
    
    BOOST_CHECK_EQUAL(delegate::read(make_global(&external_test_count,1)), 1);
  }
  {
    struct Foo {
      GlobalAddress<ConditionVariable> cv_addr;
      Foo(GlobalAddress<ConditionVariable> cv_addr): cv_addr(cv_addr) {}
      void operator()() {
        external_test_count = 2;
        signal(cv_addr);
      }
    };
    
    MessagePool pool(sizeof(Message<Foo>));
    ConditionVariable cv;
    auto cv_addr = make_global(&cv);
    
    pool.send_message(1, Foo(cv_addr));
    wait(&cv);
    
    BOOST_CHECK_EQUAL(delegate::read(make_global(&external_test_count,1)), 2);
  }
}

static int test_async_x;

void test_async_delegate() {
  BOOST_MESSAGE("Test Async delegates");
  MessagePool pool(2048);
  
  delegate::Promise<bool> a;
  a.call_async(pool, 1, []()->bool {
    test_async_x = 7;
    BOOST_MESSAGE( "x = " << test_async_x );
    return true;
  });
    
  BOOST_CHECK_EQUAL(a.get(), true);
  BOOST_CHECK_EQUAL(delegate::read(make_global(&test_async_x, 1)), 7);
  
  BOOST_MESSAGE("Testing reuse...");
  
  pool.block_until_all_sent();
  
  BOOST_CHECK_EQUAL(pool.remaining(), 2048);
  
  delegate::Promise<bool> b;
  b.call_async(pool, 1, []()->bool {
    test_async_x = 8;
    return true;
  });
    
  BOOST_CHECK_EQUAL(b.get(), true);
  BOOST_CHECK_EQUAL(delegate::read(make_global(&test_async_x, 1)), 8);
  
}

void test_overrun() {
  BOOST_MESSAGE("Testing overrun.");
    
  struct RemoteWrite {
    GlobalAddress<FullEmpty<int64_t>> a;
    RemoteWrite(FullEmpty<int64_t> * x): a(make_global(x)) {}
    void operator()() {
      auto aa = a;
      send_heap_message(a.core(), [aa] {
        aa.pointer()->writeXF(1);
      });
    }
  };
  
  MessagePoolStatic<sizeof(Message<RemoteWrite>)> pool;
  
  FullEmpty<int64_t> x;
  FullEmpty<int64_t> y;
  
  pool.send_message(1, RemoteWrite(&x));
  
  // should block... (use '--vmodule MessagePool=3' to verify)
  pool.send_message(1, RemoteWrite(&y));
  
  BOOST_CHECK_EQUAL(x.readFF(), 1);
  BOOST_CHECK_EQUAL(y.readFF(), 1);
}

void user_main(void* ignore) {
  test_pool1();
  test_pool2();
  test_pool_external();
  test_async_delegate();
  test_overrun();
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
