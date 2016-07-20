// to run, do something like
// make -j demo-hpcc_random_access
// bin/grappa_run --ppn 8 --nnode 12 -- demo-hpcc_random_access.exe 

#include <Grappa.hpp>
#include <iomanip>

using namespace Grappa;

// define command-line flags (third-party 'gflags' library)
DEFINE_int64( scale, 20, "table size = 2 ^ scale * sizeof(uint64_t) * number of PEs" );
DEFINE_int64( iters, 4, "Number of iterations" );

// define custom statistics which are logged by the runtime
// (here we're not using these features, just printing them ourselves)
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_runtime, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, gups_throughput, 0.0 );

#define POLY 0x0000000000000007ULL
#define PERIOD 1317624576693539401LL

typedef double time_type;
std::string print_time(time_type t)
{
	std::ostringstream out;
	out << std::setiosflags(std::ios::fixed) << std::setprecision(2) << t;
	return out.str();
}


uint64_t N;
uint64_t HPCC_starts(int64_t n);

Grappa::GlobalCompletionEvent randomaccess_gce;

template <Grappa::GlobalCompletionEvent * GCE = &randomaccess_gce >
	double run_random_access() {
		LOG(INFO) << "HPCC RandomAccess" << std::endl;
		N = (1 << FLAGS_scale) * cores();

		// create target array that we'll be updating
		auto hpcc_table = global_alloc<int64_t>(N);
		Grappa::memset( hpcc_table, 0, N);

		double tstart = walltime();

		on_all_cores([hpcc_table] {
			//for(int k = 0; k < FLAGS_iters; k++)
			//forall_here<async>(0, FLAGS_iters, [=](uint64_t k) {
			forall_here<Grappa::SyncMode::Async>(0, FLAGS_iters, [=](int64_t k) {
					uint64_t key = HPCC_starts((FLAGS_iters + k)* mycore() * N / cores());
					for(uint64_t i = 0; i < N / cores(); i++) {
						key = key << 1 ^ ((int64_t) key < 0 ? POLY : 0);
						auto addr = hpcc_table + (key & N - 1);
						//auto core = key >> (int)log2((double)(N / cores())) & cores() - 1;
						delegate::call<async, GCE>(addr.core(), [addr, key] {
							//uint64_t offset = key & (N / cores() - 1);
							*(addr.pointer()) ^= key;
						});
					}
				});
		});

		double tend = walltime();

		LOG(INFO) << "\tTime elapsed " << (double)(tend - tstart) << " sec" << std::endl;

		global_free(hpcc_table);

		return (double)(tend - tstart);
	}

uint64_t HPCC_starts(int64_t n) {
	int i, j;
	uint64_t m2[64];
	uint64_t temp, ran;
	while (n < 0) n += PERIOD;
	while (n > PERIOD) n -= PERIOD;
	if (n == 0) return 0x1;
	temp = 0x1;
	for (i = 0; i < 64; i++) {
		m2[i] = temp;
		temp = temp << 1 ^ ((int64_t) temp < 0 ? POLY : 0);
		temp = temp << 1 ^ ((int64_t) temp < 0 ? POLY : 0);
	}
	for (i = 62; i >= 0; i--)
		if (n >> i & 1)
			break;

	ran = 0x2;
	while (i > 0) {
		temp = 0;
		for (j = 0; j < 64; j++)
			if (ran >> j & 1)
				temp ^= m2[j];
		ran = temp;
		i -= 1;
		if (n >> i & 1)
			ran = ran << 1 ^ ((int64_t) ran < 0 ? POLY : 0);
	}
	return ran;
}


int main(int argc, char * argv[]) {
  init( &argc, &argv );
  run([]{

		LOG(INFO) << "\tGlobal table size   = 2^" << FLAGS_scale << " * " << cores() << " = " << (1 << FLAGS_scale) * cores() << " words\n";
		LOG(INFO) << "\tNumber of processes = " << cores() << std::endl;
		LOG(INFO) << "\tNumber of updates = " << FLAGS_iters * (1 << FLAGS_scale) * cores() << std::endl;

		gups_runtime = run_random_access();
    gups_throughput = 1e-9 * FLAGS_iters * N / gups_runtime;

    LOG(INFO) << "[Final] CPU time used " << print_time(gups_runtime.value()) << " seconds, " << print_time(gups_throughput.value()) << " GUPS\n";
    
  });
  finalize();
}
