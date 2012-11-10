

...

int x = 4;
GlobalAddress<int> x_addr = make_global(&x);

// remote task
{
  // read value
  int x_value = Grappa_delegate_read(x_addr);

  // remote increment
  Grappa_delegate_fetch_add(x_addr, 1);
}
