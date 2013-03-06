#include <mpi.h>
#include <sys/time.h> 
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <sstream>
#include "../CombBLAS.h"
#include "../Applications/TwitterEdge.h"

using namespace std;

// One edge = 16 bytes (rounded up from 11)
// One parent = 8 bytes
#define INC 256
#define L1 4096	// maximum entries of edges+parents combined to fit 32 KB
#define REPEAT 1000




// all the local variables before the EWiseApply wouldn't be accessible, 
// so we need to pass them as parameters to the functor constructor
class twitter_mult : public std::binary_function<ParentType, TwitterEdge, ParentType>
{
private:
	time_t sincedate;
public:
	twitter_mult(time_t since):sincedate(since) {};
  ParentType operator()(const ParentType & arg1, const TwitterEdge & arg2) const
  {
	if(arg2.isFollower() && arg2.TweetSince(sincedate))      
		return arg1;
	else
		return ParentType();    // null-type parent id
  }
};


int main(int argc, char* argv[])
{
	int nprocs, myrank;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
	
	{
		int64_t len = INC;
		time_t now;
		time ( &now );
		TwitterEdge twe(4, 1, now);	// 4 retweets, latest now, following

		while(len < L1)
		{
			//  FullyDistVec(IT globallen, NT initval)
			FullyDistVec<int64_t,TwitterEdge> tvec(nprocs * len, twe);
			FullyDistVec<int64_t,ParentType> pvec;
			pvec.iota(nprocs * len, ParentType());
	
			MPI_Barrier(MPI_COMM_WORLD);
			double t1 = MPI_Wtime(); 	// initilize (wall-clock) timer

			time_t now = time(0);
			struct tm * timeinfo = localtime( &now);
			timeinfo->tm_mon = timeinfo->tm_mon-1;
			time_t monthago = mktime(timeinfo);
			
			for(int i=0; i< REPEAT; ++i)
				pvec.EWiseApply(tvec, twitter_mult(monthago));

			MPI_Barrier(MPI_COMM_WORLD);
			double t2 = MPI_Wtime(); 	
			
			if(myrank == 0)
			{
				cout<<"EWiseApply Iterations finished"<<endl;	
				double time = t2-t1;
				double teps = (nprocs*len*REPEAT) / (time * 1000000);
				printf("%.6lf seconds elapsed for %d iterations on vector of length %lld\n", time, REPEAT, nprocs*len);
				printf("%.6lf million TEPS per second\n", teps);
			}
			len += INC;
		}
	}
	MPI_Finalize();
	return 0;
}
