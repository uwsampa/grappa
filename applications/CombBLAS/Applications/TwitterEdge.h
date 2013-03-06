#ifndef _TWITTER_EDGE_
#define _TWITTER_EDGE_

#include <iostream>
#include <ctime>
#include "../CombBLAS.h"

using namespace std;

struct DetSymmetricize;

/**
 * This is not part of the Combinatorial BLAS library, it's just an example.
 * A nonzero A(i,j) is present if at least one edge (of any type)
 * from vertex i to vertex j exists. Multiple types of edges are supported: follower/retweeter (or both)
 **/
class TwitterEdge
{
public:
	TwitterEdge(): count(0), follower(0), latest(0) {};
	template <typename X>
	TwitterEdge(X x):count(0), follower(0), latest(0) {};	// any upcasting constructs the default object too

	TwitterEdge(short mycount, bool myfollow, time_t mylatest):count(mycount), follower(myfollow), latest(mylatest) {};
	bool isFollower() const {	return follower; };
	bool isRetwitter() const {	return (count > 0); };
	bool TweetWithinInterval (time_t begin, time_t end) const	{	return ((count > 0) && (begin <= latest && latest <= end));  };
	bool TweetSince (time_t begin) const	{	return ((count > 0) && (begin <= latest));  };
	bool LastTweetBy (time_t end) const	{	return ((count > 0) && (latest <= end));  };

	operator bool () const	{	return true;	} ;       // Type conversion operator (ABAB: Shoots in the foot by implicitly converting many things)

	TwitterEdge & operator+=(const TwitterEdge & rhs) 
	{
		cout << "Error: TwitterEdge::operator+=() shouldn't be executed" << endl;
		count += rhs.count;
		follower |= rhs.follower;
		if(rhs.count > 0)	// ensure that addition with additive identity doesn't change "latest"
			latest = max(latest, rhs.latest);
		return *this;
	}
	bool operator ==(const TwitterEdge & b) const
	{
		return ((follower == b.follower) && (latest == b.latest) &&  (count == b.count));
	}

	friend ostream& operator<<( ostream& os, const TwitterEdge & twe);
	friend TwitterEdge operator*( const TwitterEdge & a, const TwitterEdge & b);

private:
	bool follower;		// default constructor sets all to zero
	time_t latest;		// not assigned if no retweets happened
	short count;		
	
	template <typename IT>
	friend class TwitterReadSaveHandler;
	
	friend struct DetSymmetricize;
};

ostream& operator<<(ostream& os, const TwitterEdge & twe )    
{      
	if( twe.follower == 0 && twe.latest == 0 &&  twe.count == 0)
		os << 0;
	else
		os << 1;
	return os;    
};

TwitterEdge operator*( const TwitterEdge & a, const TwitterEdge & b)
{
	// One of the parameters is an upcast from bool (used in Indexing), so return the other one
	if(a == TwitterEdge())	return b;
	else	return a;
}


template <class IT>
class TwitterReadSaveHandler
{
	public:
		TwitterReadSaveHandler() {};
		TwitterEdge getNoNum(IT row, IT col) { return TwitterEdge(); }

		MPI_Datatype getMPIType()
		{
			return MPIType<TwitterEdge>(); // utilize the MPI type cache
		}

		void binaryfill(FILE * rFile, IT & row, IT & col, TwitterEdge & val)
		{
			TwitterInteraction twi;
			size_t entryLength = fread (&twi,sizeof(TwitterInteraction),1,rFile);
			row = twi.from - 1 ;
			col = twi.to - 1;
			val = TwitterEdge(twi.retweets, twi.follow, twi.twtime); 
			if(entryLength != 1)
				cout << "Not enough bytes read in binaryfill " << endl;
		}
		size_t entrylength() { return sizeof(TwitterInteraction); }

		template <typename c, typename t>
		TwitterEdge read(std::basic_istream<c,t>& is, IT row, IT col)
		{
			TwitterEdge tw;
			is >> tw.follower;
			is >> tw.count;
			if(tw.count > 0)
			{
				string date;
				string time;
				is >> date;
				is >> time;

				struct tm timeinfo;
				int year, month, day, hour, min, sec;
				sscanf (date.c_str(),"%d-%d-%d",&year, &month, &day);
				sscanf (time.c_str(),"%d:%d:%d",&hour, &min, &sec);

				memset(&timeinfo, 0, sizeof(struct tm));
				timeinfo.tm_year = year - 1900; // year is "years since 1900"
				timeinfo.tm_mon = month - 1 ;   // month is in range 0...11
				timeinfo.tm_mday = day;         // range 1...31
				timeinfo.tm_hour = hour;        // range 0...23
				timeinfo.tm_min = min;          // range 0...59
				timeinfo.tm_sec = sec;          // range 0.
				tw.latest = timegm(&timeinfo);
				if(tw.latest == -1) { cout << "Can not parse time date" << endl; exit(-1);}
			}
			else
			{
				tw.latest = 0;	// initialized to dummy
			}
			//cout << row << " follows " << col << "? : " << tw.follower << " and the retweet count is " << tw.count << endl;
			return tw;
		}
		
	
		template <typename c, typename t>
		void save(std::basic_ostream<c,t>& os, const TwitterEdge & tw, IT row, IT col)	// save is NOT compatible with read
		{
			os << row << "\t" << col << "\t";
			os << tw.follower << "\t";
			os << tw.count << "\t";
			os << tw.latest << endl;
		}
	private:
		struct TwitterInteraction
		{
     		  	int32_t from;
      		  	int32_t to;
      		  	bool follow;
        		int16_t retweets;
        		time_t twtime;
		};
};


struct ParentType
{
	ParentType():id(-1) { };
	ParentType(int64_t myid):id(myid) { };
	int64_t id;
	bool operator ==(const ParentType & rhs) const
	{
		return (id == rhs.id);
	}
	bool operator !=(const ParentType & rhs) const
	{
		return (id != rhs.id);
	}
	ParentType & operator+=(const ParentType & rhs) 
	{
		cout << "Adding parent with id: " << rhs.id << " to this one with id " << id << endl;
		return *this;
	}
	const ParentType operator++(int)	// for iota
	{
		ParentType temp(*this);	// post-fix requirement
		++id;
		return temp;
	}
	friend ostream& operator<<(ostream& os, const ParentType & twe ); 

	template <typename IT>
	friend ParentType operator+( const IT & left, const ParentType & right); 
};

ostream& operator<<(ostream& os, const ParentType & twe )    
{      
	os << "Parent=" << twe.id;
	return os;    
};

ParentType NumSetter(ParentType & num, int64_t index) 
{
	return ParentType(index);
}

template <typename IT>
ParentType operator+( const IT & left, const ParentType & right)
{
	return ParentType(left+right.id);
}

// forward declaration
template <typename SR, typename T>
void select2nd(void * invec, void * inoutvec, int * len, MPI_Datatype *datatype);


template <typename SR, typename VECTYPE>
static VECTYPE filtered_select2nd(const TwitterEdge & arg1, const VECTYPE & arg2, time_t & sincedate)  
{
	if(sincedate == -1)	// uninitialized
	{
		struct tm timeinfo;
		memset(&timeinfo, 0, sizeof(struct tm));
		int year, month, day, hour, min, sec;
		year = 2009;	month = 7;	day = 1;
		hour = 0;	min = 0;	sec = 0;
		
		timeinfo.tm_year = year - 1900; // year is "years since 1900"
		timeinfo.tm_mon = month - 1 ;   // month is in range 0...11
		timeinfo.tm_mday = day;         // range 1...31
		timeinfo.tm_hour = hour;        // range 0...23
		timeinfo.tm_min = min;          // range 0...59
		timeinfo.tm_sec = sec;          // range 0.
		sincedate = timegm(&timeinfo);
		
		ostringstream outs;
		outs << "Initializing since date (only once) to " << sincedate << endl;
		SpParHelper::Print(outs.str());
	}
	
	if(arg1.isRetwitter() && arg1.LastTweetBy(sincedate))	// T1 is of type edges for BFS
	{
		return arg2;
	}
	else
	{
		SR::returnedSAID(true);
		return VECTYPE();	
		// return null-type parent id (for BFS) or 
		// double() for MIS - POD objects are zero initilied
	}
}



//! Filters are only indirectly supported in Combinatorial BLAS
//! KDT generates them by embedding their filter stack and pushing those
//! predicates to the SR::multiply() function, conceptually like 
//! if(predicate(maxrix_val)) { bin_op(xxx) }
//! Here we emulate this filtered traversal approach.
struct LatestRetwitterBFS
{
	static MPI_Op MPI_BFSADD;
	static ParentType id() { return ParentType(); }	// additive identity

	// the default argument means that this function can be used like this:
	// if (returnedSAID()) {...}
	// which is how it is called inside CombBLAS routines. That call conveniently clears the flag for us.
	static bool returnedSAID(bool setFlagTo = false) 
	{
		static bool flag = false;

		bool temp = flag; // save the current flag value to be returned later. Saves an if statement.
		flag = setFlagTo; // set/clear the flag.
		return temp;
	}

	static ParentType add(const ParentType & arg1, const ParentType & arg2)
	{
		return ((arg2 == ParentType()) ? arg1: arg2);
	}

	static MPI_Op mpi_op() 
	{ 
		MPI_Op_create(select2nd<LatestRetwitterBFS,ParentType>, false, &MPI_BFSADD);	// \todo {do this once only, by greating a MPI_Op buffer}
		return MPI_BFSADD;
	}
	static time_t sincedate;
	static ParentType multiply(const TwitterEdge & arg1, const ParentType & arg2)
	{
		return filtered_select2nd<LatestRetwitterBFS>(arg1, arg2, sincedate);
	}
	static void axpy(TwitterEdge a, const ParentType & x, ParentType & y)
	{
		y = add(y, multiply(a, x));
	}
};

time_t LatestRetwitterBFS::sincedate = -1;

// select2nd for doubles
template <typename SR, typename T>
void select2nd(void * invec, void * inoutvec, int * len, MPI_Datatype *datatype)
{
	T * pinvec = static_cast<T*>(invec);
	T * pinoutvec = static_cast<T*>(inoutvec);
	for (int i = 0; i < *len; i++)
	{
		pinoutvec[i] = SR::add(pinvec[i], pinoutvec[i]);
	}
}


MPI_Op LatestRetwitterBFS::MPI_BFSADD;

struct getfringe: public std::binary_function<ParentType, ParentType, ParentType>
{
  	ParentType operator()(ParentType x, const ParentType & y) const
	{
		return x;
	}
	
};

// x: Parent type (always 1 if exits, sparse)
// y: degree (dense)
struct seldegree: public std::binary_function<ParentType, int64_t, int64_t>
{
  	int64_t operator()(ParentType x, const int64_t & y) const
	{
		return y;
	}
	
};

// This is like an "isparentset" with the extra parameter that we don't care
struct passifthere: public std::binary_function<ParentType, int64_t, bool>
{
  	bool operator()(ParentType x, const int64_t & y) const
	{
		return  (x != ParentType());
	}
	
};

// DoOp for MIS's EWiseApply
struct is2ndSmaller: public std::binary_function<double, double, bool>
{
  	bool operator()(double m, double c) const
	{
		return (c < m);
	}
};

// BinOp for MIS's EWiseApply
struct return1_uint8: public std::binary_function<double, double, uint8_t>
{
	uint8_t operator() (double t1, double t2)
	{
		return (uint8_t) 1;
	}
};


// x: elements from fringe (sparse), y: elements from parents (dense) 
// return true for edges that are not filtered out, and not previously discovered
// if the edge was filtered out, then x would be ParentType()
// if y was already discovered its parent would NOT be ParentType()
struct keepinfrontier_f: public std::binary_function<ParentType, ParentType, bool>
{
  	bool operator()(ParentType x, const ParentType & y) const
	{
		return ( x != ParentType() && y == ParentType()) ;
	}
	
};

struct isparentset: public std::unary_function<ParentType, bool>
{
  	bool operator()(const ParentType & x) const
	{
		return ( x != ParentType() ) ;
	}
	
};

// Matrix type: TwitterEdge
// Vector type: double
struct LatestRetwitterMIS
{
	static double id() { return 0.0; }	// additive identity
	
	// the default argument means that this function can be used like this:
	// if (returnedSAID()) {...}
	// which is how it is called inside CombBLAS routines. That call conveniently clears the flag for us.
	static bool returnedSAID(bool setFlagTo = false) 
	{
		static bool flag = false;
		
		bool temp = flag; // save the current flag value to be returned later. Saves an if statement.
		flag = setFlagTo; // set/clear the flag.
		return temp;
	}
	
	static double add(const double & arg1, const double & arg2)
	{
		return std::min(arg1, arg2);
	}
	
	static MPI_Op mpi_op() 
	{ 
		return MPI_MIN;
	}
	static time_t sincedate;
	static double multiply(const TwitterEdge & arg1, const double & arg2)  // filtered select2nd
	{
		return filtered_select2nd<LatestRetwitterMIS>(arg1, arg2, sincedate);
	}
	static void axpy(TwitterEdge a, const double & x, double & y)
	{
		y = add(y, multiply(a, x));
	}
};

// Matrix type: TwitterEdge
// Vector type: double
struct LatestRetwitterSelect2nd // also used for finding neighbors of the candidate set in MIS
{
	static MPI_Op MPI_SEL2NDADD;
	static double id() { return 0.0; }	// additive identity
	
	// the default argument means that this function can be used like this:
	// if (returnedSAID()) {...}
	// which is how it is called inside CombBLAS routines. That call conveniently clears the flag for us.
	static bool returnedSAID(bool setFlagTo = false) 
	{
		static bool flag = false;
		
		bool temp = flag; // save the current flag value to be returned later. Saves an if statement.
		flag = setFlagTo; // set/clear the flag.
		return temp;
	}
	
	static double add(const double & arg1, const double & arg2)
	{
		return arg2;
	}
	
	static MPI_Op mpi_op() 
	{ 
		MPI_Op_create(select2nd<LatestRetwitterSelect2nd,double>, false, &MPI_SEL2NDADD);	// \todo {do this once only, by greating a MPI_Op buffer}
		return MPI_SEL2NDADD;
	}
	static time_t sincedate;
	static double multiply(const TwitterEdge & arg1, const double & arg2)  // filtered select2nd
	{
		return filtered_select2nd<LatestRetwitterSelect2nd>(arg1, arg2, sincedate);
	}
	static void axpy(TwitterEdge a, const double & x, double & y)
	{
		y = add(y, multiply(a, x));
	}
};

time_t LatestRetwitterMIS::sincedate = -1;
time_t LatestRetwitterSelect2nd::sincedate = -1;
MPI_Op LatestRetwitterSelect2nd::MPI_SEL2NDADD;



#endif
