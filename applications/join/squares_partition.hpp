#pragma once
#include "Query.hpp"
#include "grappa/graph.hpp"

class SquarePartition4way: public Query {
  private:
    GlobalAddress<Graph<Vertex>> index; 
  public:
    virtual void preprocessing(std::vector<tuple_graph> relations);

    virtual void execute(std::vector<tuple_graph> relations);
};
