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

#include "StealQueue.hpp"
#include "Metrics.hpp"
#include "DictOut.hpp"


/* metrics */

// work steal network usage
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, stealq_reply_messages, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, stealq_reply_total_bytes, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, stealq_request_messages, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, stealq_request_total_bytes, 0);

// work share network usage 
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, workshare_request_messages, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, workshare_request_total_bytes, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, workshare_reply_messages, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, workshare_reply_total_bytes, 0);

// work share elements transfered
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, workshare_request_elements_denied, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, workshare_request_elements_received, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, workshare_reply_elements_sent, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, workshare_requests_client_smaller, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, workshare_requests_client_larger, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, workshare_reply_nacks, 0);

// global queue data transfer network usage
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, globalq_data_pull_request_messages, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, globalq_data_pull_request_total_bytes, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, globalq_data_pull_reply_messages, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, globalq_data_pull_reply_total_bytes, 0);

// global queue elements transfered
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, globalq_data_pull_request_num_elements, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, globalq_data_pull_reply_num_elements, 0);


namespace Grappa {

namespace impl {


void StealMetrics::record_steal_reply( size_t msg_bytes ) {
  stealq_reply_messages += 1;
  stealq_reply_total_bytes += msg_bytes;
}

void StealMetrics::record_steal_request( size_t msg_bytes ) {
  stealq_request_messages += 1;
  stealq_request_total_bytes += msg_bytes;
}

void StealMetrics::record_workshare_request( size_t msg_bytes ) {
  workshare_request_messages += 1;
  workshare_request_total_bytes += msg_bytes;
}

void StealMetrics::record_workshare_reply( size_t msg_bytes, bool isAccepted, int num_received, int num_denying, int num_sending ) {
  workshare_reply_messages += 1;
  workshare_reply_total_bytes += msg_bytes;

  if ( isAccepted ) {
    workshare_requests_client_smaller += 1;
  } else {
    workshare_requests_client_larger += 1;
  }

  workshare_request_elements_received+=( num_received );
  workshare_request_elements_denied+=( num_denying );
  workshare_reply_elements_sent+=( num_sending );
}

void StealMetrics::record_workshare_reply_nack( size_t msg_bytes ) {
  workshare_reply_messages += 1;
  workshare_reply_total_bytes += msg_bytes;

  workshare_reply_nacks += 1;
}

void StealMetrics::record_globalq_data_pull_request( size_t msg_bytes, uint64_t amount ) {
  globalq_data_pull_request_messages += 1;
  globalq_data_pull_request_total_bytes += msg_bytes;

  globalq_data_pull_request_num_elements+=amount ;
}

void StealMetrics::record_globalq_data_pull_reply( size_t msg_bytes, uint64_t amount ) {
  globalq_data_pull_reply_messages += 1;
  globalq_data_pull_reply_total_bytes += msg_bytes;

  globalq_data_pull_reply_num_elements +=  amount ;
}


} // namespace impl
} // namespace Grappa
