// spawn task on each core, equivalent to fork_join_custom:
on_all_cores([]{
  // do work
});

// custom delegate
GlobalAddress<int> x;
// fetch and xor
int val = delegate::call(x.core(), [x]{
  int old = x.pointer();
  *x.pointer() = old ^ 12345;
  return old;
});

// loop decomposition (local), roughly equivalent to `async_parallel_for`
GlobalAddress<int> X, Y;
forall_here(0, N, [](int start, int niterations){
  // do stuff, e.g.:
  for (int i=start; i<start+niterations; i++) {
    int value = delegate::read(X+i);
    delegate::write(Y+i, value);
  }
});

// localized loop (runs iterations where the elements of the array reside)
GlobalAddress<double> array;
forall_localized(array, N, [](int index, double& element){
  element = 1.0 / index;
});

// Asynchronous delegates
// If you want the value, then you must use the future/promise version:
// (this is not in the strictest sense what we have, but should be possible with move semantics)
delegate::Promise<double> p = delegate::read_async(array+3);
// do other stuff
double value = p.get(); // blocks until delegate completes

// if you don't need the return value and want to delay synchronization to the GlobalCompletionEvent, then:
// (assume GCE is a global static GlobalCompletionEvent)
for (int i=0; i<N; i++) {
  delegate::write_async<GCE>(array+i, 4);
}
GCE.wait();

// or if you're using a global parallel loop, it'll use the implicit GCE of the loop:
// (assume GlobalAddress<ptrdiff_t> indexes is declared)
forall_localized(indexes, N, [array](int i, ptrdiff_t& e){
  delegate::write_async(array+e, 1.0/i);
}); // all writes completed




//
// task spawns
//
// these use c++ lambdas to capture arguments if necessary.
//

// spawn a private task
int arg = 123;

spawn_private_task( [arg] { // capture arg by value in lambda
    do_work(arg);
  });

spawn_private_task( [&arg] { // capture arg by reference in lambda
    do_work(arg);
  });

// spawn a private task
spawn_public_task( [captured_variables] {
    do_work(captured_variables);
  });

// to spawn a remote private task, just send a message with the spawn inside
auto m = send_message( destination, [arg] {
    spawn_private_task( [arg] {
        do_work(arg);
      });
  });









//
// sync ops
// 
// these can all be accessed locally or through delegate ops
// 


// 
// Mutex
//
// this is included for completeness--I don't think we've ever used it.
//

Mutex m;

lock( &m );
bool success = trylock( &m );
unlock( &m );



//
// Semaphore
//
// This is a counting semaphore that blocks if a decrement would leave it negative.
//

Semaphore s;

s.increment(); // increment by 1, waking waiters if it goes positive
s.increment(10); // increment by 10, waking waiters if it goes positive

s.decrement(); // decrement by 1, blocking if it would be left negative
s.decrement(3); // decrement by 3, blocking if it would be left negative

bool success = s.try_decrement(); // try to decrement by 1, returning false if it would block

int v = s.get_value(); // just return count

// this could be written like this for consistency
increment( &s );
decrement( &s, 5);
// etc

//
// Condition Variables
//
// these are our basic syncronization primitive. 
//

ConditionVariable cv;
Mutex m; // a mutex is not necessary in our system due to our mostly atomic execution. 

wait( &cv ); // block until signaled

wait( &cv, &m ); // block until signaled, releasing and reacquiring mutex

signal( &cv ); // wake one waiter blocked on cv
broadcast( &cv ); // wake all waiters blocked on cv

// we don't have a satisfying story on blocking on remote condition
// variables right now: currently if you want to do that, you would
// have to spawn a task on the home node of the CV that blocks on the
// CV; when it is woken it would then send a message to wake the
// original requester. As you might imagine, we don't usually do this.



//
// full bits wrap a data value, adding an additional 64 bits to store
// the state of the full bit along with a queue of waiting workers.
//

FullEmpty< int64_t > x; // initially empty

x.writeXF( 1 ); // doesn't block; just modifies value, leaving it full, and wakes waiters
x.writeFF( 2 ); // blocks until full, modifies value, leaves full, (wakes waiters, but is it necessary?)
x.writeEF( 3 ); // blocks until empty, modifies value, leaves full, wakes waiters

int64_t v1 = x.readXX(); // doesn't block or signal, just returns value (not used much)
int64_t v2 = x.readFF(); // blocks until full, returns value, leaves full, (wakes waiters, but is it necessary?)
int64_t v1 = x.readFE(); // blocks until full, returns value, leaves empty, wakes waiters

bool is_full = x.full();
bool is_empty = x.empty();

x.reset(); // discards state and waiters and leaves empty.

// we could also present these in a more MTA-like style like this, if
// it makes more sense with other stuff. This would allow us to take
// GlobalAddress<FullEmpty<T>> arguments for remote operations
writeXF( &x, 1 );



//
// completion events
//
// the api similar to the global completion event, but these don't
// have to communicate between nodes. note that if you're mixing
// enroll and complete, you need to make sure your enrolls stay ahead
// of your completes so you don't actually finish early.
//

CompletionEvent ce;

// we will wait for 5 children
ce.enroll( 5 );

// spawn children
for( int i = 0; i < 5; ++i ) {
  // these are local private tasks, so we can capture the address of the completion event by reference
  spawn_private_task( [&ce] {
      do_work();
      ce.complete();
    });
 }

ce.wait(); // blocks until all children have completed.
