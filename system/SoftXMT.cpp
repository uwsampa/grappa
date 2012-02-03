
#include "SoftXMT.hpp"

static Communicator * my_global_communicator;
static Aggregator * my_global_aggregator;

void SoftXMT_init( int * argc_p, char ** argv_p[] )
{
  // also initializes system_wide global_communicator pointer
  my_global_communicator = new Communicator();
  my_global_communicator->init( argc_p, argv_p );

  // also initializes system_wide global_aggregator pointer
  my_global_aggregator = new Aggregator( my_global_communicator );

}


void SoftXMT_activate() 
{
  my_global_communicator->activate();
}

Node SoftXMT_nodes() {
  return my_global_communicator->nodes();
}

Node SoftXMT_mynode() {
  return my_global_communicator->mynode();
}

void SoftXMT_barrier() {
  my_global_communicator->barrier();
}

void SoftXMT_flush( Node n )
{
  my_global_aggregator->flush( n );
}

void SoftXMT_poll()
{
  my_global_aggregator->poll();
}

void SoftXMT_finish( int retval )
{
  my_global_communicator->finish( retval );
  
  // probably never get here
  delete my_global_aggregator;
  delete my_global_communicator;
}
