
#include <iostream>

#include "SoftXMT.hpp"


// set to 1 when it's time to barrier
int barrier = 0;

// set to 1 when it's time to tear down
int done = 0;


enum Actions { Barrier, Hello, Finish };

struct done_am_args {
	Actions action;
};

void done_am( done_am_args * arg, size_t arg_size, void * payload, size_t payload_size ) {
	std::cout << "Node " << global_communicator->mynode() 
	<< " got message with value " << arg->action
	<< std::endl;
	if( arg->action == Barrier ) {
		barrier = 1;
	}
	if( arg->action == Hello ) {
		std::cout << "Node " << global_communicator->mynode() 
		<< " got message " << static_cast< char * >( payload )
		<< std::endl;
		barrier = 1;
	}
	if( arg->action == Finish ) {
		done = 1;
	}
}

int main( int argc, char * argv[] ) {
	std::cout << "Initializing..." << std::endl;
	SoftXMT_init( &argc, &argv );
	
	std::cout << "Activating..." << std::endl;
	SoftXMT_activate();
	
	std::cout << "Running user code..." << std::endl;
	if( SoftXMT_mynode() == 0 ) {
		int i;
		
		// send a message to everyone
		for( i = 0; i < SoftXMT_nodes(); ++i ) {
			done_am_args args = { Barrier };
			SoftXMT_call_on( i, &done_am, &args );
			SoftXMT_flush( i );
		}
		
		std::cout << "Barrier..." << std::endl;
		global_communicator->barrier();
		
		// tell everyone to exit
		for( i = 0; i < SoftXMT_nodes(); ++i ) {
			done_am_args args = { Hello };
			char message[10] = "Hello!";
			SoftXMT_call_on( i, &done_am, &args, sizeof(args), &message, sizeof(message) );
			SoftXMT_flush( i );
		}
		
		std::cout << "Barrier..." << std::endl;
		global_communicator->barrier();
		
		// tell everyone to exit
		for( i = 0; i < SoftXMT_nodes(); ++i ) {
			done_am_args args = { Finish };
			SoftXMT_call_on( i, &done_am, &args );
			SoftXMT_flush( i );
		}
		
		while( !done ) {
			SoftXMT_poll();
		}
	} else {
		while( !done ) {
			SoftXMT_poll();
			if( barrier ) {
				std::cout << "Barrier..." << std::endl;
				SoftXMT_barrier();
				barrier = 0;
			}
		}
	}
	
	std::cout << "Finishing..." << std::endl;
	SoftXMT_finish( 0 );
	
	// should never get here
	return 0;
}
