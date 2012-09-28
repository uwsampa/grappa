#include "SoftXMT.hpp"
#include "Tasking.hpp"
#include "Delegate.hpp"
#include "Addressing.hpp"

/// GlobalTaskJoiner functions to keep track of a number of tasks across all the nodes in the 
/// system and allows tasks to suspend waiting on all of the tasks on all the nodes completing.
/// It acts like LocalTaskJoiner but does a kind of cancellable barrier on a node when it has 
/// no local outstanding tasks. It is 'cancelled' if more work is stolen by the tasking layer, 
/// until all of the nodes enter the barrier, indicating there is no more work to be done, and
/// the waiting tasks on each node are woken.
/// 
/// This implementation essentially rolls a LocalTaskJoiner-like class with a global cancellable
/// barrier implementation. This version keeps the original waiting task suspended until consenus
/// on all nodes is reached, using AMs exclusively to do synchronization.
///
/// Assumptions:
/// - Tasks that are registered on a given node must signal that same joiner in order to 
///   guarantee correct  completion detection (so for instance, public tasks should use a 
///   GlobalAddress to do a remote signal back to its joiner in case it is stolen).
/// - A single instance of GlobalTaskJoiner (declared `joiner`) exists on each node, and
///   only one join can be "in flight" at a time.
///
/// Architecture/design:
/// 
/// The concept of the "global joiner" is made up of one global instance of a
/// GlobalTaskJoiner object on each node, but one node's joiner (Node 0) is
/// considered the 'master' node that the other joiners check in with. Whenever
/// a node runs out of tasks to run, it enters a "cancellable barrier" and lets
/// the master joiner know. If that node manages to steal some more work, it
/// notifies the master that it's no longer idle. If all of the nodes report being
/// idle, then the master knows that there is no more work to do in this phase and
/// notifies everyone to wake.
struct GlobalTaskJoiner {
  // Master barrier
  Node nodes_in;
  bool barrier_done; // only for our own verification

  // All nodes
  Thread * waiter;
  int64_t outstanding;
  bool global_done;
  bool cancel_in_flight;
  bool enter_called;
 
  // Const
  Node target;
  GlobalAddress<GlobalTaskJoiner> _addr;
  
  GlobalTaskJoiner(): nodes_in(0), barrier_done(false), waiter(NULL), outstanding(0), global_done(false), cancel_in_flight(false), enter_called(false), target(0 /* Node 0 == Master */) {}
  void reset();
  GlobalAddress<GlobalTaskJoiner> addr() {
    if (_addr.pointer() == NULL) { _addr = make_global(this); }
    return _addr;
  }
  void registerTask();
  void signal();
  void wait();
  static void remoteSignal(GlobalAddress<GlobalTaskJoiner> joiner);
  static void remoteSignalNode(Node joiner_node);
private:
  void wake();
  void send_enter();
  void send_cancel();
  static void am_wake(bool * ignore, size_t sz, void * p, size_t psz);
  static void am_enter(bool * ignore, size_t sz, void * p, size_t psz);
  static void am_remoteSignal(GlobalAddress<GlobalTaskJoiner>* joiner, size_t sz, void* payload, size_t psz);
  static void am_remoteSignalNode(int* dummy_arg, size_t sz, void* payload, size_t psz);
};
extern GlobalTaskJoiner global_joiner;

//
// Spawning tasks that use the global_joiner
//
#include "AsyncParallelFor.hpp"

template < void (*LoopBody)(int64_t,int64_t),
           int64_t Threshold >
void joinerSpawn( int64_t s, int64_t n );

/// task wrapper: signal upon user task completion
template < void (*LoopBody)(int64_t,int64_t),
           int64_t Threshold >
void asyncFor_with_globalTaskJoiner(int64_t s, int64_t n, GlobalAddress<GlobalTaskJoiner> joiner) {
  //NOTE: really we just need the joiner Node because of the static global_joiner
  async_parallel_for<LoopBody, &joinerSpawn<LoopBody,Threshold>, Threshold > (s, n);
  DVLOG(5) << "signaling " << s << " " << n;
  global_joiner.remoteSignal( joiner );
}

/// spawn wrapper: register new task before spawned
template < void (*LoopBody)(int64_t,int64_t),
           int64_t Threshold >
void joinerSpawn( int64_t s, int64_t n ) {
  global_joiner.registerTask();
  DVLOG(5) << "registered " << s << " " << n;
  SoftXMT_publicTask(&asyncFor_with_globalTaskJoiner<LoopBody,Threshold>, s, n, make_global( &global_joiner ) );
}



template < typename Arg,
           void (*LoopBody)(int64_t,int64_t,GlobalAddress<Arg>),
           int64_t Threshold >
void joinerSpawn_hack( int64_t s, int64_t n, GlobalAddress<Arg> shared_arg );

/// task wrapper: signal upon user task completion
template < typename Arg,
           void (*LoopBody)(int64_t,int64_t,GlobalAddress<Arg>),
           int64_t Threshold >
void asyncFor_with_globalTaskJoiner_hack(int64_t s, int64_t n, GlobalAddress<Arg> shared_arg ) {
  async_parallel_for<Arg, LoopBody, &joinerSpawn_hack<Arg, LoopBody, Threshold>, Threshold > (s, n, shared_arg);
  global_joiner.remoteSignalNode( shared_arg.node() ); 
}

template < typename Arg,
           void (*LoopBody)(int64_t,int64_t,GlobalAddress<Arg>),
           int64_t Threshold >
void joinerSpawn_hack( int64_t s, int64_t n, GlobalAddress<Arg> shared_arg ) {
  global_joiner.registerTask();

  // copy the shared_arg data into a global address that corresponds to this Node
  GlobalAddress<Arg> packed = make_global( reinterpret_cast<Arg*>(shared_arg.pointer()) );
  SoftXMT_publicTask( &asyncFor_with_globalTaskJoiner_hack<Arg,LoopBody,Threshold>, s, n, packed );
}

/// Does a global join phase with starting iterations of the for loop split in blocks
/// among all the nodes.
/// ALLNODES (to be called from within a fork_join_custom setting by all nodes)
#define global_async_parallel_for(f, g_start, g_iters) \
{ \
  range_t r = blockDist(g_start, g_start+g_iters, SoftXMT_mynode(), SoftXMT_nodes()); \
  global_joiner.reset(); \
  async_parallel_for<f, joinerSpawn<f,ASYNC_PAR_FOR_DEFAULT>, ASYNC_PAR_FOR_DEFAULT >(r.start, r.end-r.start); \
  global_joiner.wait(); \
}


/// Like global_async_parallel_for but allows you to specify a non-standard threshold.
/// @see global_async_parallel_for 
#define global_async_parallel_for_thresh(f, g_start, g_iters, static_threshold) \
{ \
  range_t r = blockDist(g_start, g_start+g_iters, SoftXMT_mynode(), SoftXMT_nodes()); \
  global_joiner.reset(); \
  async_parallel_for<f, joinerSpawn<f,static_threshold>,static_threshold>(r.start, r.end-r.start); \
  global_joiner.wait(); \
}

/// Makes it easier to call async_parallel_for with an additional value (using the 'hack').
#define async_parallel_for_hack(f, start, iters, value) \
{ \
  GlobalAddress<void*> packed = make_global( (void**)(value) ); \
  async_parallel_for<void*, f, joinerSpawn_hack<void*,f, ASYNC_PAR_FOR_DEFAULT>, ASYNC_PAR_FOR_DEFAULT >(start, iters, packed); \
}

#include "Delegate.hpp"

template< typename S, typename T, void (*BinOp)(S&,const T&)>
static void am_ff_delegate(GlobalAddress<S>* target_back, size_t tsz, void* payload, size_t psz) {
  CHECK(psz == sizeof(T));
  CHECK(tsz == sizeof(GlobalAddress<S>));

  T* val = reinterpret_cast<T*>(payload);

  BinOp(*target_back->pointer(), *val);

  // signal back to the caller's joiner
  GlobalTaskJoiner::remoteSignalNode(target_back->node());
}

/// Feed-forward version of a generic delegate. "Feed forward" delegates are a kind of 
/// split-phase delegate that doesn't guarantee completion until the next GlobalTaskJoin
/// completes. This works by 'registering' the delegate with the node's GlobalTaskJoiner,
/// so its completion is guaranteed by the same mechanism that tasks are.
/// 
/// @tparam S Type of the target argument.
/// @tparam T Type of the value argument which will be sent over to the target. Note: cannot be larger than the max AM size (to be safe, < 2048 bytes).
/// @tparam BinOp Function to run at the remote node, will be passed a pointer to the
///               target and the value sent with the delegate.
/// @param target GlobalAddress that specifies where and what to run the delegate op on.
/// @param val Value to be used to do the delegate computation.
template< typename S, typename T, void (*BinOp)(S&, const T&) >
void ff_delegate(GlobalAddress<S> target, const T& val) {
  if (target.node() == SoftXMT_mynode()) {
    BinOp(*target.pointer(), val);
  } else {
    global_joiner.registerTask();

    // store return node & remote pointer in the GlobalAddress
    GlobalAddress<S> back = make_global(target.pointer());
    SoftXMT_call_on(target.node(), &am_ff_delegate<S,T,BinOp>, &back, sizeof(GlobalAddress<S>), &val, sizeof(T));
  }
}
/// Overload of ff_delegate with same target type and value type. @see ff_delegate()
template<typename T, void (*BinOp)(T&,const T&)>
void ff_delegate(GlobalAddress<T> target, const T& val) {
  ff_delegate<T,T,BinOp>(target,val);
}

/// BinOp template parameter to ff_delegate that does a simple increment.
template< typename T >
inline void ff_add(T& target, const T& val) {
  target += val;
}

/// BinOp template parameter to ff_delegate that does a simple write over the target location.
template< typename T >
inline void ff_write(T& target, const T& val) {
  target = val;
}

/// Feed-forward delegate increment implemented on top of the generic ff_delegate.
/// @see ff_delegate()
template< typename T >
inline void ff_delegate_add(GlobalAddress<T> target, const T& val) {
  ff_delegate<T, ff_add<T> >(target, val);
}

/// Feed-forward delegate write implemented on top of the generic ff_delegate.
/// @see ff_delegate()
template< typename T >
inline void ff_delegate_write(GlobalAddress<T> target, const T& val) {
  ff_delegate<T, ff_write<T> >(target, val);
}

