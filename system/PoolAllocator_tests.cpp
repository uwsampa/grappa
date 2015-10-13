////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#include <Grappa.hpp>
#include <Addressing.hpp>
#include <Message.hpp>
#include <MessagePool.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>

#include <boost/test/unit_test.hpp>

using namespace Grappa;
using Grappa::wait;

BOOST_AUTO_TEST_SUITE( PoolAllocator_tests );

void test_pool1() {
  BOOST_MESSAGE("Test MessagePool");
  CompletionEvent joiner;
  
  int x[10];

  {
    MessagePoolStatic<2048> pool;
    
    for (int i=0; i<10; i++) {
      joiner.enroll();
      auto* f = pool.message(1, [i,&x,&joiner]{
        send_heap_message(0, [i,&x, &joiner]{
          x[i] = i;
          joiner.complete();
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
  CompletionEvent joiner;
  
  int x[10];
  MessagePoolStatic<2048> pool;

  for (int i=0; i<10; i++) {
    joiner.enroll();
    auto f = pool.send_message(1, [i,&x,&joiner]{
      send_heap_message(0, [i,&x, &joiner]{
        x[i] = i;
        joiner.complete();
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
  
  delegate::Promise<bool> a;
  a.call_async(1, []()->bool {
    test_async_x = 7;
    BOOST_MESSAGE( "x = " << test_async_x );
    return true;
  });
    
  BOOST_CHECK_EQUAL(a.get(), true);
  BOOST_CHECK_EQUAL(delegate::read(make_global(&test_async_x, 1)), 7);
  
  BOOST_MESSAGE("Testing reuse...");
  
  delegate::Promise<bool> b;
  b.call_async(1, []()->bool {
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

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    test_pool1();
    test_pool2();
    test_pool_external();
    test_async_delegate();
    test_overrun();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
