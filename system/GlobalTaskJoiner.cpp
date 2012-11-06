
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "GlobalTaskJoiner.hpp"

GlobalTaskJoiner global_joiner;


/// Reset all of the settings for the task joiner. Does a Grappa_barrier, so this must be called
/// by all nodes in order for any to finish.
/// ALLNODES
void GlobalTaskJoiner::reset() {
  VLOG(2) << "reset";
  // only matters for Master
  nodes_in = 0;
  barrier_done = false;
 
  // all
  global_done = false;
  outstanding = 0;
  waiter = NULL;
  cancel_in_flight = false;
  enter_called = false;

  Grappa_barrier_suspending();
  VLOG(2) << "barrier_done = " << barrier_done;
}

/// Tell joiner that there is one more task outstanding.
void GlobalTaskJoiner::registerTask() {
  outstanding++;
  if (outstanding == 1) { // 0 -> 1
    send_cancel();
  }
  VLOG(3) << "registered - outstanding = " << outstanding;
}

/// Signal joiner that a task has completed. If this was the last outstanding task
/// on this node, then this jonter the cancellable barrier.
void GlobalTaskJoiner::signal() {
  CHECK(outstanding > 0) << "too many calls to signal(): outstanding == " << outstanding;
  
  outstanding--;
  VLOG(3) << "signaled - outstanding = " << outstanding;

  if (outstanding == 0) { // 1 -> 0
    // TODO: shouldn't be necessary, but because we don't 'count' the original task, we have to assume it's still running. If this is the case, the original task will call 'send_enter()' before waiting
    if (waiter != NULL) {
      send_enter();
    }
  }
}

/// Wake this node's suspended task and set joiner to the completed state to ensure that
/// any subsequent calls to wait() will fall through (until reset() is called of course).
void GlobalTaskJoiner::wake() {
  CHECK(!global_done);
  CHECK(outstanding == 0) << "outstanding = " << outstanding;
  VLOG(2) << "wake!";
  global_done = true;
  
  if ( waiter != NULL ) {
    Grappa_wake(waiter);
    waiter = NULL;
  }
}

/// Suspend current task waiting for all of the tasks registered on any global
/// joiner signal completion. This works similarly to LocalTaskJoiner's wait,
/// but also ensures that it won't wake until the global joiners on other nodes
/// have also received all their signals.
/// 
/// It is assumed that this will be called once on each node.
///
/// ALLNODES
void GlobalTaskJoiner::wait() {
  if (!global_done) {
    if (!cancel_in_flight && outstanding == 0 && !enter_called) {
      // TODO: doing extra 'cancel_in_flight' check
      VLOG(2) << "no local work, entering cancellable barrier";
      send_enter();
    }
    waiter = CURRENT_THREAD;
    Grappa_suspend();
    CHECK(global_done);
    CHECK(!cancel_in_flight);
    CHECK(outstanding == 0);
  }
}

void GlobalTaskJoiner::am_remoteSignal(GlobalAddress<GlobalTaskJoiner>* joiner, size_t sz, void* payload, size_t psz) {
  CHECK(joiner->node() == Grappa_mynode());
  //joiner->pointer()->signal();
  global_joiner.signal(); // equivalent to above, right now
}

/// Signal completion of a task to a joiner (potentially on another node). Note: it will
/// short-circuit and signal without sending a message if the joiner is local.
///
/// @param joiner This GlobalAddress is only used for its node (the single joiner on the
///               target node is called, rather than relying on the pointer() part of 
///               the address).
void GlobalTaskJoiner::remoteSignal(GlobalAddress<GlobalTaskJoiner> joiner) {
  if (joiner.node() == Grappa_mynode()) {
    //joiner.pointer()->signal();
    global_joiner.signal(); // equivalent to above, right now
  } else {
    VLOG(2) << "remoteSignal -> " << joiner.node();
    Grappa_call_on(joiner.node(), &GlobalTaskJoiner::am_remoteSignal, &joiner);
  }
}

// version of remote signal that takes the Node of the joiner only
void GlobalTaskJoiner::am_remoteSignalNode(int* dummy_arg, size_t sz, void* payload, size_t psz) {
  global_joiner.signal(); 
}

/// Signal completion of a task to a joiner (potentially on another node). Note: it will
/// short-circuit and signal without sending a message if the joiner is local.
///
/// @param joiner The node where the joiner to signal lives. The single instance of
///               the joiner will be found.
void GlobalTaskJoiner::remoteSignalNode(Node joiner_node) {
  if (joiner_node == Grappa_mynode()) {
    global_joiner.signal(); 
  } else {
    VLOG(2) << "remoteSignalNode -> " << joiner_node;
    int dummy_arg = -1;
    Grappa_call_on(joiner_node, &GlobalTaskJoiner::am_remoteSignalNode, &dummy_arg);
  }
}

void GlobalTaskJoiner::am_wake(bool * ignore, size_t sz, void * p, size_t psz) {
  global_joiner.wake();
}

void GlobalTaskJoiner::am_enter(bool * ignore, size_t sz, void * p, size_t psz) {
  CHECK(Grappa_mynode() == 0); // Node 0 is Master

  global_joiner.nodes_in++;
  VLOG(2) << "(completed) nodes_in: " << global_joiner.nodes_in;
  
  if (global_joiner.nodes_in == Grappa_nodes()) {
    global_joiner.barrier_done = true;
    VLOG(2) << "### done! ### nodes_in: " << global_joiner.nodes_in;
    for (Node n=0; n<Grappa_nodes(); n++) {
      bool ignore = false;
      Grappa_call_on(n, &GlobalTaskJoiner::am_wake, &ignore);
    }
  }
}

/// (internal) Notify the master joiner that this joiner doesn't have any work to do for now.
void GlobalTaskJoiner::send_enter() {
  enter_called = true;
  if (!cancel_in_flight) {
    VLOG(2) << "notify_completed";
    bool ignore = false;
    Grappa_call_on(target, &GlobalTaskJoiner::am_enter, &ignore);
  }
}

/// (internal) Let the master joiner know that this joiner has stolen more work
/// and is no longer waiting in the barrier.
void GlobalTaskJoiner::send_cancel() {
  if (!cancel_in_flight && enter_called) {
    cancel_in_flight = true;
    enter_called = false;

    GlobalAddress<GlobalTaskJoiner> global_joiner_addr = make_global(&global_joiner, target);
    GlobalAddress<int64_t> global_nodes_in_addr = global_pointer_to_member(global_joiner_addr, &GlobalTaskJoiner::nodes_in);
    int64_t result = Grappa_delegate_fetch_and_add_word(global_nodes_in_addr, -1);
    VLOG(2) << "(cancelled) nodes_in: " << result-1;

    cancel_in_flight = false;
   
    CHECK(outstanding > 0);
  }
}

