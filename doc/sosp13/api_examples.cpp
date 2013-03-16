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
