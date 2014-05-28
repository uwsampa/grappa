#include <Grappa.hpp>
#include <graph/Graph.hpp>
#include "lubyspan.hpp"
#include <random>

DEFINE_int32(scale, 8, "Log2 number of vertices.");
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

/*inline bool spanning(GlobalAddress<Graph<BFSVertex>> g) {
  forall(g, [g](BFSVertex &v) {
    if (v->level == 2) {
      v->seen = true;
      forall<async>(adj(g, v), [](BFSVertex &vj) {
        vj->seen = true;
      });
    }
  });

  bool ret = true;
  forall(g, [&ret] (BFSVertex &v) {
    if (!v->seen) {
      ret = false;
    }
  });

  return ret;
}*/

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
          if (v->level == 0 && (v->parent == 0 || distrib(randengine) % (2 * v->parent) == 0)) {
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
          int64_t nadj = v.nadj;
          int64_t rank = v->rank;
          auto gv = make_global( &v ); 
          forall<async>(adj(g, v), [nadj, gv, g, rank](GlobalAddress<LubyVertex> vj) {
            bool discard = delegate::call(vj, [nadj, rank](LubyVertex &l) {
              if(l->level == 2 || (l->level == 1 && l.nadj > nadj)
                  || (l->level == 1 && l.nadj == nadj && l->rank > rank)) {
        //        LOG(INFO) << "Marking " << l->parent << " as 2 at discard";
                l->level = 2;
                return true;
              } else { 
        //        LOG(INFO) << "Marking " << l->parent << " as -1 at discard";
                l->level = 0;
                return false;
              }
            });

            if(discard) {
              delegate::call( gv, []( LubyVertex &x ) {
                LOG(INFO) << "Marking " << x->rank << "as -1 due to discard"; 
                x->level = -1;
              });

              forall(adj(g, gv), [](GlobalAddress<LubyVertex> discardAdj) {
                delegate::call(discardAdj, [](LubyVertex &discardAdj_) {
                  discardAdj_->parent--;
                });
              });
            } else {
              delegate::call( gv, [] ( LubyVertex &x ) {
        //        LOG(INFO) << "Marking " << x->parent << "as 2";
                x->level = 2;
              });

              forall(adj(g, gv), [rank](GlobalAddress<LubyVertex> discardAdj) {
                delegate::call(discardAdj, [rank] (LubyVertex & discard) {
                  if (rank != discard->rank) {
                    LOG(INFO) << "Marking " << discard->rank << "as -1 due to non-discard";
                    discard->level = -1;
                  }
                });
              });
            }
          });
          
          if(v->level == -1) {
            /*forall<async>(adj(g, v), [](GlobalAddress<BFSVertex>vj) {
              delegate::call(vj, [](BFSVertex &l) {
                l->parent--;
              }); 
            });*/
          }
       
          if (v->level == 1) {
      //    LOG(INFO) << "Marking " << v->parent << " as 2 at end of loop";
            delegate::call(gv, [](LubyVertex & mark) {
              mark->level = 2;
            });
          }
        }
      });

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
