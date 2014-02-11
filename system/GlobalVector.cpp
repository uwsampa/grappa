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

#include "GlobalVector.hpp"

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_push_ops, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_push_msgs, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_pop_ops, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_pop_msgs, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_deq_ops, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_deq_msgs, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_matched_pushes, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_matched_pops, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, global_vector_push_latency, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, global_vector_pop_latency, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, global_vector_deq_latency, 0);

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, global_vector_master_combined, 0);
