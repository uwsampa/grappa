#ifndef _MPI_TYPE_H
#define _MPI_TYPE_H

#include <iostream>
#include <typeinfo>
#include <map>
#include <mpi.h>
#include <stdint.h>

using namespace std;


// A datatype cache inspired by (mostly copied from) Boost http://www.boost.org/LICENSE_1_0.txt)

/// @brief comparison function object for two std::type_info pointers
/// is implemented using the before() member function of the std::type_info class
struct type_info_compare
{
  bool operator()(std::type_info const* lhs, std::type_info const* rhs) const
  {
    return lhs->before(*rhs);
  }
};


class MPIDataTypeCache
{
private:
  typedef std::map<std::type_info const*,MPI_Datatype,type_info_compare> stored_map_type;
  stored_map_type map;

public:
  void clear()
  {
	int is_finalized=0;
	MPI_Finalized(&is_finalized);
	if (! is_finalized ) 	// do not free after call to MPI_FInalize
	{
		// ignore errors in the destructor
		for (stored_map_type::iterator it=map.begin(); it != map.end(); ++it)
		{
			MPI_Type_free(&(it->second));
		}
	}
  }
  ~MPIDataTypeCache()
  {
    	clear();
  }
  MPI_Datatype get(const std::type_info* t)
  {
      	stored_map_type::iterator pos = map.find(t);
      	if (pos != map.end())
          	return pos->second;
      	else
        	return MPI_DATATYPE_NULL;
  }

  void set(const std::type_info* t, MPI_Datatype datatype)
  {
     	 map[t] = datatype;
  }

  MPIDataTypeCache& mpi_datatype_cache()
  {
    	static MPIDataTypeCache cache;
    	return cache;
  }
};


/**
  * C++ type to MPIType conversion is done through functions returning the mpi types
  * The templated function is explicitly instantiated for every C++ type 
  * that has a correspoinding MPI type. For all others, a data type is created
  * assuming it's some sort of struct. Each created data type is committed only once
  **/

extern MPIDataTypeCache mpidtc;	// global variable
// Global variables have program scope, which means they can be accessed everywhere in the program, and they are only destroyed when the program ends.

template <typename T> 
MPI_Datatype MPIType ( void )
{
	std::type_info const* t = &typeid(T);
	MPI_Datatype datatype = mpidtc.get(t);

	if (datatype == MPI_DATATYPE_NULL) 
	{
		MPI_Type_contiguous(sizeof(T), MPI_CHAR, &datatype );
		MPI_Type_commit(&datatype);
		int myrank;
		MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
		if(myrank == 0)
			cout << "Creating a new MPI data type for " << t->name() << endl;
		mpidtc.set(t, datatype);
	}
   	return datatype;
};

template<> MPI_Datatype MPIType< signed char >( void );
template<> MPI_Datatype MPIType< signed short int >( void );
template<> MPI_Datatype MPIType< unsigned char >( void );
template<> MPI_Datatype MPIType< unsigned short int >( void );
template<> MPI_Datatype MPIType< int32_t >( void );
template<> MPI_Datatype MPIType< uint32_t >( void );
template<> MPI_Datatype MPIType< int64_t >( void );
template<> MPI_Datatype MPIType< uint64_t >( void );
template<> MPI_Datatype MPIType< float >( void );
template<> MPI_Datatype MPIType< double >( void );
template<> MPI_Datatype MPIType< long double >( void );
template<> MPI_Datatype MPIType< bool >( void );



#endif
