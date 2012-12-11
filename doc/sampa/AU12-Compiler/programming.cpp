// global allocator
GlobalAddress<int> a = Grapp_malloc<int>(N);

// delegate operations

// simple memory ops
int a_val = delegate_read(a);
delegate_write(a, 3);

// also synchronization ops
delegate_read_

