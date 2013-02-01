
void sum(GlobalAddress<int> array, int i, GlobalAddress<int> total) {
  
}

void user_main() {
  int N = 1024;
  GlobalAddress<int> array = Grappa::malloc<int>(N);

  int total;

  Grappa_forall(sum, array, N, make_global(&total));

  
}
