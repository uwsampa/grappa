////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

#pragma once

#include <Addressing.hpp>
#include <GlobalAllocator.hpp>

namespace Grappa {

  class TupleGraph {
  public:    
    struct Edge { int64_t v0, v1; };

  private:
    bool initialized;

    static TupleGraph load_generic( std::string, void (*f)( const char *, Edge*, Edge*) );
    void save_generic( std::string, void (*f)( const char *, Edge*, Edge*) );
    
    static TupleGraph load_tsv( std::string path );
    
  public:
    GlobalAddress<Edge> edges;
    int64_t nedge; /* Number of edges in graph, in both cases */
  
    /// Use Graph500 Kronecker generator (@see graph/KroneckerGenerator.cpp)
    static TupleGraph Kronecker(int scale, int64_t desired_nedge, 
                                         uint64_t seed1, uint64_t seed2);

    // create new TupleGraph with edges loaded from file
    static TupleGraph Load( std::string path, std::string format );

     void destroy() { global_free(edges); }

    // default contstructor
    TupleGraph()
      : initialized( false )
      , edges( )
      , nedge(0)
    { }

    TupleGraph(const TupleGraph& tg): initialized(false), edges(tg.edges), nedge(tg.nedge) { }

    TupleGraph& operator=(const TupleGraph& tg) {
      if( initialized ) {
        global_free<Edge>(edges);
      }
      edges = tg.edges;
      nedge = tg.nedge;
      return *this;
    }

    void save( std::string path, std::string format );

  protected:
    TupleGraph(int64_t nedge): initialized(true), edges(global_alloc<Edge>(nedge)), nedge(nedge) {}
    
  };

}
