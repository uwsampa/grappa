//
//  test.cpp
//  SoftXMT-Sampa
//
//  Created by Brandon Holt on 2/28/12.
//  Copyright (c) 2012 University of Washington. All rights reserved.
//

#include "SoftXMT.hpp"
#include "Allocator.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"
#include "common.hpp"
#include "timer.h"

#define ID "<" << SoftXMT_mynode() << "> "

thread * main_thread = NULL;

int replies = 0;

struct empty {};
static void am_reply(empty* a, size_t sz, void* p, size_t psz) {
  replies++;
  SoftXMT_wake(main_thread);
}

struct am_test_args {
  int caller;
};
static void th_test(thread * me, am_test_args * a) {
  LOG(INFO) << ID << "thread test";
  SoftXMT_call_on(a->caller, &am_reply, new empty);
  SoftXMT_flush(a->caller);
}

static void user_main(thread * me, void * args) {
  main_thread = get_current_thread();

  am_test_args a = { SoftXMT_mynode() };
  SoftXMT_remote_spawn(&th_test, &a, 0);
  SoftXMT_remote_spawn(&th_test, &a, 1);
//  SoftXMT_flush(0);
//  SoftXMT_flush(1);
  
  while (replies < 2) {
    DVLOG(1) << "waiting for replies (" << replies << "/" << 2 << " so far)";
    SoftXMT_suspend();
  }
  
  SoftXMT_signal_done();
}

int main(int argc, char * argv[]) {
  SoftXMT_init(&argc, &argv);
  SoftXMT_activate();
  
  SoftXMT_run_user_main(&user_main, NULL);
  
  LOG(INFO) << "finishing";
	SoftXMT_finish( 0 );
	// should never get here
	return 0;
}
