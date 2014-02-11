#pragma once
#include "Query.hpp"
#include "grappa/graph.hpp"

class SquareBushyPlan : public Query {
  public:
    virtual void preprocessing(std::vector<tuple_graph> relations);

    virtual void execute(std::vector<tuple_graph> relations);
};
