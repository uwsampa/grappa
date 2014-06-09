#include <Grappa.hpp>
#include <graph/Graph.hpp>
#include "lubyspan.hpp"
#include <random>

DEFINE_int32(scale, 20, "Log2 number of vertices.");
DEFINE_int32(edgefactor, 4, "Average edges per vertex.");

std::default_random_engine randengine;
std::uniform_int_distribution<unsigned int> distrib;

// level: 0 indicates not candidate not in set
// 	  1 indicates candidate
// 	 -1 indicates discarded
// 	  2 indicates accepted

bool unconnected(GlobalAddress<Graph<LubyVertex>> graph) {
  forall(graph, [](LubyVertex &vertextoclear) {
    vertextoclear->seen = false;
  });

  forall(graph, [graph](LubyVertex &vertextocheck) {
    if (vertextocheck->level == 2) {
      forall<async>(adj(graph, vertextocheck), [](GlobalAddress<LubyVertex> gvertex) {
        delegate::call<async>(gvertex, [](LubyVertex &gvertex_) {
          gvertex_->seen = true;
        });
      }); 
    }
  });

  int64_t connected = 0;
  auto conglobal = make_global(&connected);
  forall(graph, [conglobal](LubyVertex &vertextocheck) {
    if (vertextocheck->level == -1 && vertextocheck->seen == false) {
      LOG(INFO) << "vertex " << vertextocheck->rank << " discarded unseen";
      delegate::call(conglobal, [](int64_t &conglobal_) {
        conglobal_++;
      });; 
    }
  });
  return connected == 0;
}

inline int64_t stillVertices(GlobalAddress<Graph<LubyVertex>> g) {
 int64_t numRemaining = 0;
 
 auto numr = make_global(&numRemaining);
  forall(g, [numr, g](LubyVertex &v) {
    if(v->level == 0) {
      delegate::call(numr, [](int64_t &nr) {
        nr++;
      });
    } else if (v->level == 1) {
      LOG(INFO) << "Error, level of 1 at " << v->rank;
    }
  });

  unconnected(g);
  LOG(INFO) << "Num remaining = " << numRemaining; 
  return numRemaining;
}

int main(int argc, char * argv[]) {
  init(&argc, &argv);
  run([] {
    int64_t steps = 0;
    int64_t NE = (1L << FLAGS_scale) * FLAGS_edgefactor;
    auto tg = TupleGraph::Kronecker(FLAGS_scale, NE, 111, 222);
    auto g = Graph<LubyVertex>::create(tg);

    int64_t vertexnum = 0;
    auto vernum = make_global(&vertexnum);
    forall(g, [vernum](LubyVertex& v) {
      v->init();
      v->level = 0;
      v->parent = v.nadj;
      v->rank = delegate::call(vernum, [](int64_t &vernum_) {
        vernum_++;
        return vernum_;
      });
    });

    int64_t numRemaining = 25;
    do {
      LOG(INFO) << "Executing step " << steps; 
      steps++;

      if (numRemaining > 15) {
        forall(g, [](LubyVertex &v) {
          if (v->level == 0 && (v->parent <= 0 || distrib(randengine) % (2 * v->parent) == 0)) {
            v->level = 1; // candidate
          }
        });
      } else {
        forall(g, [](LubyVertex &v) {
          if (v->level == 0) {
      //      LOG(INFO) << "Marking " << v->parent << " as 1";
            v->level = 1;
          }
        });
      }

      forall(g, [g](LubyVertex &v) {
        if (v->level == 1) {
          int64_t nadj = v->parent;
          int64_t rank = v->rank;

          forall<async>(adj(g, v), [nadj, rank](GlobalAddress<LubyVertex> l) {
            delegate::call<async>(l, [nadj, rank](LubyVertex &l_) {
              if(l_->level == 1 && (l_->parent < nadj || (
                l_->parent == nadj && l_->rank < rank))) {
                l_->level = 0;
              }
            });
          });
        }
      });

      forall(g, [g](LubyVertex &v) {
        if (v->level == 1) {
          v->level = 2;
          int64_t rank = v->rank;
          forall<async>(adj(g, v), [rank, g](GlobalAddress<LubyVertex> discardAdj) {
            delegate::call<async>(discardAdj, [rank, g](LubyVertex &discardAdj_) {
              if (rank != discardAdj_->rank) {
                CHECK(discardAdj_->level <= 0) << "Discarding edge with val " << discardAdj_->level;
                discardAdj_->level = -1;
                Grappa::spawn([g, discardAdj_] {
                  forall<async>(adj(g, discardAdj_),
                    [](GlobalAddress<LubyVertex> dec) {
                    delegate::call<async>(dec, [](LubyVertex &dec_) {
                      dec_->parent--;
                    });
                  });
                });
              }
            });
          });
        }
      });

      LOG(INFO) << " Finished executing step " << steps;
      //sync();
      numRemaining = stillVertices(g);
    } while (numRemaining > 0);

     forall(g, [g](LubyVertex&v) {
       v->seen = false;
     });

    //bool print = spanning(g);
      forall(g, [g](LubyVertex &v) {
        if (v->level == 2) {  
          v->seen = true;
          forall<async>(adj(g, v), [](GlobalAddress<LubyVertex> vj) {
            delegate::call(vj, [](LubyVertex &l) {
              l->seen = true;
            });
          });
        }
      });

    /*forall(g, [g](BFSVertex &v) {
      if (v->level == 2) {
        v->seen = true;
        forall<async>(adj(g, v), [](GlobalAddress<BFSVertex> &vj) {
          delegate::call(vj, [](BFSVertex &l) {
            l->seen = true;
          });
        });
      }
    });*/

    bool spanning = true;
    forall(g, [&spanning](LubyVertex &v) {
      if (v->seen == false) {
        LOG(INFO) << "Not seen " << v->parent;
        spanning = false;
      }
    });

    if(spanning) {
      LOG(INFO) << "Yay! Spanning maybe.";
    } else {
      LOG(INFO) << "Not actually a spanning set";
    }

    tg.destroy();
    g->destroy();
  });
  finalize();
}
