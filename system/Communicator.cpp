
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <cassert>
#include <sstream>
#include <fstream>

#include <gflags/gflags.h>

#ifdef HEAPCHECK_ENABLE
#include <gperftools/heap-checker.h>
extern HeapLeakChecker * Grappa_heapchecker;
#endif

#include "Communicator.hpp"

DEFINE_string( route_graph_filename, "routing.dot", "Name of file for routing graph" );

/// Global communicator instance
Communicator global_communicator;

/// Construct communicator
Communicator::Communicator( )
  : handlers_()
  , registration_is_allowed_( false )
  , communication_is_allowed_( false )
  , mycore_( -1 )
  , mylocale_( -1 )
  , locale_mycore_( -1 )
  , cores_( -1 )
  , locales_( -1 )
  , locale_cores_( -1 )
  , locale_of_core_(NULL)
  , source_core_for_locale_(NULL)
  , dest_core_for_locale_(NULL)
#ifdef VTRACE_FULL
  , communicator_grp_vt( VT_COUNT_GROUP_DEF( "Communicator" ) )
  , send_ev_vt( VT_COUNT_DEF( "Sends", "bytes", VT_COUNT_TYPE_UNSIGNED, communicator_grp_vt ) )
#endif
  , stats()
{ }


/// initialize communication layer. After this call, node parameters
/// may be queried and handlers may be registered, but no
/// communication is allowed.
void Communicator::init( int * argc_p, char ** argv_p[] ) {
#ifdef HEAPCHECK_ENABLE
  {
    HeapLeakChecker::Disabler disable_leak_checks_here;
#endif
  GASNET_CHECK( gasnet_init( argc_p, argv_p ) ); 
#ifdef HEAPCHECK_ENABLE
  }
#endif
  // make sure the Node type is big enough for our system
  assert( static_cast< int64_t >( gasnet_nodes() ) <= (1L << sizeof(Node) * 8) && 
          "Node type is too small for number of nodes in job" );

  // initialize job geometry
  mycore_ = gasnet_mynode();
  cores_ = gasnet_nodes();

  mylocale_ = gasneti_nodeinfo[mycore_].supernode;
  locales_ = gasnet_nodes() / gasneti_nodemap_local_count;
  locale_mycore_ = gasneti_nodemap_local_rank;
  locale_cores_ = gasneti_nodemap_local_count;

  // allocate and initialize core-to-locale translation
  locale_of_core_ = new Locale[ cores_ ];
  for( int i = 0; i < cores_; ++i ) {
    locale_of_core_[i] = gasneti_nodeinfo[i].supernode;
  }

  Core locale_first_core = mylocale_ * locale_cores_;
  Core locale_last_core = mylocale_ * locale_cores_ + locale_cores_;

  // allocate and initialize locale-to-core translation
  if( locale_mycore_ == 0 ) {
    // how many locales should each core handle?
    // (assume all locales have same core count)
    //    int probable_locales_per_core = (locales_ - 1) / locale_cores_;
    int probable_locales_per_core = locales_ / locale_cores_;
    int locales_per_core = probable_locales_per_core > 0 ? probable_locales_per_core : 1;
    LOG(INFO) << "probable_locales_per_core " << probable_locales_per_core
              << " locales_per_core " << locales_per_core;

    // initialize source cores
    // try to assign the same number of destinations per node
    source_core_for_locale_ = new Core[ locales_ ];
    int current_core = mylocale_ * locale_cores_; // init to first core of locale
    int uses = 0;
    // for( int i = 0; i < locales_; ++i ) {
    //   if( i == mylocale_ ) {
    //     source_core_for_locale_[i] = -1;
    //   } else {
    //     source_core_for_locale_[i] = current_core;
    //     uses++;
    //     if( (uses == locales_per_core) && (current_core < locale_last_core - 1) ) {
    //       current_core++;
    //       uses = 0;
    //     }
    //   }
    // }

    for( int i = 0; i < locales_; ++i ) {
      if( i == mylocale_ ) {
        source_core_for_locale_[i] = -1;
      } else {
        source_core_for_locale_[i] = locale_first_core + i / locales_per_core;
      }
    }

    dest_core_for_locale_ = new Core[ locales_ ];
    for( int i = 0; i < locales_; ++i ) {
      if( i == mylocale_ ) {
        dest_core_for_locale_[i] = -1;
      } else {
        //dest_core_for_locale_[i] = i * locales_per_core + source_core_for_locale_[i];
        dest_core_for_locale_[i] = i * locale_cores_ + mylocale_ / locales_per_core;
        //dest_core_for_locale_[i] = i * locale_cores_ + source_core_for_locale_[i];
        //dest_core_for_locale_[i] = i * locale_cores_ + locale_mycore_;
      }
    }

    for( int i = 0; i < locales_; ++i ) {
      LOG(INFO) << "From locale " << mylocale_ << " to locale " << i 
                << " source core " << source_core_for_locale_[i]
                << " dest core " << dest_core_for_locale_[i];
    }

  }


  LOG(INFO) << " mycore_ " << mycore_ 
            << " cores_ " << cores_
            << " mylocale_ " << mylocale_ 
            << " locales_ " << locales_ 
            << " locale_mycore_ " << locale_mycore_ 
            << " locale_cores_ " << locale_cores_;

  registration_is_allowed_ = true;
}

#define DOTOUT LOG(INFO) << "DOT"
void Communicator::draw_routing_graph() {
  {
    if( mycore_ == 0 ) {
      std::ofstream o( FLAGS_route_graph_filename, std::ios::trunc );
      o << "digraph Routing {\n";
      o << "    node [shape=record];\n";
      o << "    splines=true;\n";
    }
  }
  barrier();

  for( int n = 0; n < locales_; ++n ) {
    if( locale_mycore_ == 0 && n == mylocale_ ) {
      std::ofstream o( FLAGS_route_graph_filename, std::ios::app );

      o << "    n" << n << " [label=\"n" << n;
      for( int c = n * locale_cores_; c < (n+1) * locale_cores_; ++c ) {
        o << "|";
        if( c == n * locale_cores_ ) o << "{";
        o << "<c" << c << "> c" << c;
      }
      o << "}\"];\n";

      //for( int c = n * locale_cores_; c < (n+1) * locale_cores_; ++c ) {
      for( int d = 0; d < locales_; ++d ) {
        if( d != mylocale_ ) {
          o << "    n" << n << ":c" << source_core_for_locale_[d] << ":e"
            << " -> n" << d << ":c" << dest_core_for_locale_[d] << ":w"
            << " [headlabel=\"c" << source_core_for_locale_[d] << "\"]"
            << ";\n";
        }
      }

    }
    barrier();
  }

  {
    if( mycore_ == 0 ) {
      std::ofstream o( FLAGS_route_graph_filename, std::ios::app );
      o << "}\n";
    }
  }
}


#define ONE_MEGA (1024 * 1024)
#define SHARED_PROCESS_MEMORY_SIZE  (0 * ONE_MEGA)
#define SHARED_PROCESS_MEMORY_OFFSET (0 * ONE_MEGA)
/// activate communication layer. finishes registering handlers and
/// any shared memory segment. After this call, network communication
/// is allowed, but no more handler registrations are allowed.
void Communicator::activate() {
    assert( registration_is_allowed_ );
  GASNET_CHECK( gasnet_attach( &handlers_[0], handlers_.size(), // install active message handlers
                               SHARED_PROCESS_MEMORY_SIZE,
                               SHARED_PROCESS_MEMORY_OFFSET ) );
  stats.reset_clock();
  registration_is_allowed_ = false;
  communication_is_allowed_ = true;
  LOG(INFO) << "Entering activation barrier";
  barrier();
  LOG(INFO) << "Leaving activation barrier";
  draw_routing_graph();
}

/// tear down communication layer.
void Communicator::finish(int retval) {
  communication_is_allowed_ = false;


  if( source_core_for_locale_ ) delete [] source_core_for_locale_;
  if( dest_core_for_locale_ ) delete [] dest_core_for_locale_;

  // TODO: for now, don't call gasnet exit. should we in future?
  //gasnet_exit( retval );
}

