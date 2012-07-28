#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int main( int argc, char *argv[] )
{
   char name[MPI_MAX_PROCESSOR_NAME]; // node name from MPI 
   char buffer[100], buffer2[100], buffer3[100]; // various buffers for storing strings 
   char cmdline[25][128]; // array to store contents of infile 
   char line[128]; // used to read infile one line at a time 
   int length, rank, size, i, j;
   char filename[20];
   int num = 0;
   float times[5];
   float tmp = 0.0;
   float tmptime = 0.0;
   float sum = 0.0;
   float total = 0.0;
   float mintotal = 0.0;
   float maxtotal = 0.0;
   float avgtotal = 0.0;
   float score = 0.0;
   float minval[5];
   float maxval[5];
   float bmtotal[5];

   // List of the benchmark names to be used in output filenames 
   char cmd[7][10] = {"bmm", "tilt", "ftilt", "bs2", "cba1", "cba2", "cba3" };
   char *Month[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", 
                      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
                     };

   MPI_Init(&argc, &argv);  // start MPI 
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);	// get current process id 
   MPI_Comm_size (MPI_COMM_WORLD, &size);	// get number of processes 
   MPI_Get_processor_name(name, &length);  // get node name 

   if(argc < 2)
   {
        if(rank==0) printf("Usage: cpurun <path to binaries> <path to results>\n");
        exit(0);
   }
   char *strBinDir = argv[1];
   char *strResDir = argv[2];
   sprintf(buffer, "mkdir -p %s", strResDir);
   if(rank==0) system(buffer);
   if(rank==0) printf("\n"); 
   fflush( stdout );
   MPI_Barrier( MPI_COMM_WORLD );
   printf( "Process %d of %d checking in on %s\n", rank+1, size, name ); // all processes check in
   MPI_Barrier( MPI_COMM_WORLD );
   fflush( stdout );
   sleep(2);
   
   sprintf(cmdline[0], "%s/bmm 1 55179 >> %s", strBinDir, strResDir);
   sprintf(cmdline[1], "%s/tilt 101 119554831 >> %s", strBinDir, strResDir);
   sprintf(cmdline[2], "%s/ftilt 201 119554831 >> %s", strBinDir, strResDir);
   sprintf(cmdline[3], "%s/bs2 301 2575027 >> %s", strBinDir, strResDir);
   sprintf(cmdline[4], "%s/cba 401 7173289 64 64 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[5], "%s/cba 501 202323 145 2368 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[6], "%s/cba 601 3400 231 128000 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[7], "%s/gmp 701 25 8192 >> %s", strBinDir, strResDir);
//  sprintf(cmdline[8], "%s/gmp 801 174 4096 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[9], "%s/gmp 901 1179 2048 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[10], "%s/gmp 1001 7340 1024 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[11], "%s/gmp 1101 47533 512 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[12], "%s/gmp 1201 239000 256 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[13], "%s/gmp 1301 992850 128 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[14], "%s/xor 1401 81 33554432 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[15], "%s/gt 1501 112197 8 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[16], "%s/gt 1601 552 16 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[17], "%s/gt 1701 29 25 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[18], "%s/sc 1801 75412 8 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[19], "%s/sc 1901 147 16 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[20], "%s/sc 2001 11 25 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[21], "%s/a_sort 2101 11587 16 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[22], "%s/a_sort 2201 9 25 >> %s", strBinDir, strResDir);
//   sprintf(cmdline[23], "%s/xba 2301 64375678610 >> %s", strBinDir, strResDir);

   MPI_Barrier( MPI_COMM_WORLD ); // synchronize before command execution 
   sleep(2);
   if( rank == 0 ) printf("\nRunning Benchmarks....\n");
   for( i = 0; i < 5; i++ ) // iterate through benchmark commands in order 
   {
      sprintf( buffer, "%s/timing_results_r_%d", cmdline[i], rank+1 ); // create commandline for benchmark execution
      if( rank == 0 ) printf( "%s\n", buffer );
      system( buffer ); // execute benchmarks 
      MPI_Barrier( MPI_COMM_WORLD );
      fflush( stdout );
   }
   
   if( rank == 0 ) printf("\nProcessing Results....\n");

   char filename1[20];
   char trash1[20];
   char trash2[128];
   char *token;
   char *search = " ";

   sprintf(filename1, "%s/timing_results_r_%d", strResDir, rank+1);
   FILE *file1 = fopen(filename1, "r");
   if ( file1 != NULL )
   {
      i=0;
      while( fgets( line, sizeof line, file1 ) != NULL ) // read one line at a time 
      {
         if(strstr(line, "wall time") != NULL) //look for "wall time" in the string
         {
            token = strtok(line, search);
            token = strtok(NULL, search);
            if(EOF == sscanf(token, "%f", &tmp))
            {
               printf("ERROR in sscanf\n");
            }
            times[i] = tmp;
            sum += times[i];
            i++;
         }
      }
      fclose( file1 ); // close input file 
   }
   else
   {
      perror( filename1 ); // why didn't the file open? 
   }


   score = 3600.0/(.49*sum);
   char filename2 [50];
   struct tm* tm;
   time_t now;
   now = time(0); // get current time
   tm = localtime(&now); // get structure
   MPI_Barrier( MPI_COMM_WORLD );
   if(rank == 0) 
sprintf(filename2, "%s/node_%02d%s%04d:%02d:%02d", strResDir, tm->tm_mday, Month[tm->tm_mon], tm->tm_year+1900, tm->tm_hour, tm->tm_min);

   MPI_Bcast(filename2, 50, MPI_CHARACTER, 0, MPI_COMM_WORLD);
   FILE *fp = fopen(filename2, "a");
   if(fp != NULL)
   {
      fprintf(fp, "%d, %s, %f\n", rank, name, score);
      fclose(fp); 
   } 
   else 
   {
      perror(filename2);
   }

   MPI_Reduce(&sum, &total, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
   MPI_Reduce(&sum, &mintotal, 1, MPI_FLOAT, MPI_MIN, 0, MPI_COMM_WORLD);
   MPI_Reduce(&sum, &maxtotal, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD);
   MPI_Reduce(times, minval, 5, MPI_FLOAT, MPI_MIN, 0, MPI_COMM_WORLD);
   MPI_Reduce(times, maxval, 5, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD);
   MPI_Reduce(times, bmtotal, 5, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
   if (rank == 0 ) printf("\nBenchmark ---->          Min           AVG           MAX\n");
   for(i=0; i<5; i++)
   {
      if (rank == 0 ) printf("%9s ---->  %13.3f %13.3f %13.3f\n", cmd[i], minval[i], bmtotal[i]/(float)size, maxval[i]);
   }
      avgtotal = total / (float) size;
      if (rank == 0 ) printf("Min = %5.3f seconds = %5.2f Suites/HR/Core\nAVG = %5.3f seconds = %5.2f Suites/HR/Core\nMAX = %5.3f seconds = %5.2f Suites/HR/Core\n", mintotal, 3600/(.49*mintotal), avgtotal, 3600.0/(.49*avgtotal), maxtotal, 3600.0/(.49*maxtotal));
      if(rank == 0) printf("CPU Score = %5.2f Suites/HR/Core\n", 3600.0/(.49*avgtotal));
      if(rank == 0) printf("System Score = %5.2f Suites/HR\n", size*3600.0/(.49*avgtotal));
  
   MPI_Barrier( MPI_COMM_WORLD );
   MPI_Finalize(); // stop MPI 
   return 0;
}
