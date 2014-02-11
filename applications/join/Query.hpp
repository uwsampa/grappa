#pragma once

#include <Metrics.hpp>

struct tuple_graph;

// Interface for queries
class Query {
  public:
    virtual void preprocessing(std::vector<tuple_graph> relations) = 0;
    virtual void execute(std::vector<tuple_graph> relations) = 0;
};


GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, ir1_count);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, ir2_count);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, ir3_count);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, ir4_count);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, ir5_count);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, ir6_count);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, ir7_count);

GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, results_count);

GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, ir1_final_count);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, ir2_final_count);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, ir3_final_count);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, ir4_final_count);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, ir5_final_count);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, ir6_final_count);
GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, ir7_final_count);


GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, participating_cores);

GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, edges_transfered);

GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, total_edges);


GRAPPA_DECLARE_METRIC(SimpleMetric<double>, index_runtime);

std::string resultStr(std::vector<int64_t> v, int64_t size=-1);
