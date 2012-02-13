#include "SoftXMT.hpp"

static void user_main(thread * me, void * args) {
  
  
  SoftXMT_signal_done();
}

int main(int argc, char * argv[]) {
	SoftXMT_init(&argc, &argv);
  SoftXMT_activate();
  
  SoftXMT_run_user_main(&user_main, NULL);
  
  printf("<%d> finishing...\n", SoftXMT_mynode());
	SoftXMT_finish( 0 );
	// should never get here
	return 0;
}
