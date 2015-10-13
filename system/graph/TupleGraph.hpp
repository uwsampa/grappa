////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#pragma once

#include <Addressing.hpp>
#include <GlobalAllocator.hpp>

namespace Grappa {

  class TupleGraph {
  public:    
    struct Edge {
      int64_t v0;
      int64_t v1;
    };

  private:
    bool initialized;

    static TupleGraph load_generic( std::string, void (*f)( const char *, Edge*, Edge*) );
    void save_generic( std::string, void (*f)( const char *, Edge*, Edge*) );
    
    static TupleGraph load_tsv( std::string path );
    static TupleGraph load_mm( std::string path );
    
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
