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
