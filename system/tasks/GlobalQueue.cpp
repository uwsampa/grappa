#include "GlobalQueue.hpp"

GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_reserve_request_messages, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_reserve_request_total_bytes, 0);

GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_reserve_reply_messages, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_reserve_reply_total_bytes, 0);

GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_reserve_request_messages, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_reserve_request_total_bytes, 0);

GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_entry_request_messages, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_entry_request_total_bytes, 0);

GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_reserve_reply_messages, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_reserve_reply_total_bytes, 0);

GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_entry_request_messages, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_entry_request_total_bytes, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_entry_reply_messages, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_entry_reply_total_bytes, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_reserve_hadConsumer, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_pull_reserve_noConsumer, 0);

GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_request_accepted, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_request_rejected, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_entry_hadConsumer, 0);
GRAPPA_DEFINE_STAT(BasicStatistic<uint64_t>, globalq_push_entry_noConsumer, 0);

