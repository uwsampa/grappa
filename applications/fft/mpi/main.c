#include <stdio.h>
#include <stdlib.h>
#include <fftw3-mpi.h>

static int rank;

double rand_double() {
  return 2.0 * ((double)rand()/RAND_MAX) - 1.0;
}

void generate_random(fftw_complex * local_data, int local_n) {
  srand(12345L*rank);
  for (int i=0; i<local_n; i++) {
    local_data[i][0] = rand_double();
    local_data[i][1] = rand_double();
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Wrong number of arguments.\nUsage: [exe] <scale> (where the number of complex elements in the fft array == 2^<scale>)");
    exit(1);
  }
  const int scale = atoi(argv[1]);
  const size_t nelem = 1L << scale;
  double t, fill_random_time, fft_time;

  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  fftw_mpi_init();

  if (rank == 0) {
    printf("### FFT Benchmark ###\n");
    printf("nelem = (1 << %ld) = %ld (%g GB)\n", scale, nelem, ((double)nelem)*sizeof(fftw_complex)/(1L<<30));
  }

  ptrdiff_t local_n_in, local_start_in, local_n_out, local_start_out;
  ptrdiff_t alloc_local = fftw_mpi_local_size_1d(nelem, MPI_COMM_WORLD, FFTW_FORWARD, FFTW_ESTIMATE,
        &local_n_in, &local_start_in, &local_n_out, &local_start_out);

  fftw_complex* local_data = fftw_alloc_complex(alloc_local);
  fftw_complex* workspace  = fftw_alloc_complex(alloc_local);

  fftw_plan plan = fftw_mpi_plan_dft_1d(nelem, local_data, workspace, MPI_COMM_WORLD, FFTW_FORWARD, FFTW_ESTIMATE);

  MPI_Barrier(MPI_COMM_WORLD);
  t = MPI_Wtime();
  generate_random(local_data, local_n_in);
  MPI_Barrier(MPI_COMM_WORLD);
  fill_random_time = MPI_Wtime() - t;

  t = MPI_Wtime();

  fftw_execute(plan);

  fft_time = MPI_Wtime() - t;
  if (rank == 0) {
    printf("fill_random_time: %g\n", fill_random_time);
    printf("fft_time: %g\n", fft_time);
    printf("fft_performance: %g\n", ((double)(nelem * scale))/fft_time);
  }

  fftw_destroy_plan(plan);
  free(local_data); free(workspace);
  fftw_mpi_cleanup();
  MPI_Finalize();
}
