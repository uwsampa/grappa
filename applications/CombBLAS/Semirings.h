
#ifndef _SEMIRINGS_H_
#define _SEMIRINGS_H_

#include <utility>
#include <climits>
#include <cmath>
#include "promote.h"

template <typename T>
struct inf_plus{
  T operator()(const T& a, const T& b) const {
	T inf = std::numeric_limits<T>::max();
    	if (a == inf || b == inf){
      		return inf;
    	}
    	return a + b;
  }
};

// This semiring is used in indexing (SpParMat::operator())
template <class OUT>
struct BoolCopy2ndSRing
{
	static OUT id() { return OUT(); }
	static bool returnedSAID() { return false; }
	static OUT add(const OUT & arg1, const OUT & arg2)
	{
		cout << "Add should not happen (BoolCopy2ndSRing)!" << endl;
		throw std::string("Add should not happen!");
		std::exit(1);
		return arg2;
	}
	static const OUT& multiply(bool arg1, const OUT & arg2)
	{
		return arg2;
	}
	static void axpy(bool a, const OUT & x, OUT & y)
	{
		y = multiply(a, x);
	}

	static MPI_Op mpi_op()
	{
		static MPI_Op mpiop;
		static bool exists = false;
		if (exists)
			return mpiop;
		else
		{
			MPI_Op_create(MPI_func, true, &mpiop);
			exists = true;
			return mpiop;
		}
	}

	static void MPI_func(void * invec, void * inoutvec, int * len, MPI_Datatype *datatype)
	{
		if (*len > 0)
		{
			cout << "MPI Add should not happen (BoolCopy2ndSRing)!" << endl;
			std::exit(1);
		}
	}
};

// This semiring is used in indexing (SpParMat::operator())
template <class OUT>
struct BoolCopy1stSRing
{
	static OUT id() { return OUT(); }
	static bool returnedSAID() { return false; }
	static OUT add(const OUT & arg1, const OUT & arg2)
	{
		cout << "Add should not happen (BoolCopy1stSRing)!" << endl;
		throw std::string("Add should not happen!");
		std::exit(1);
		return arg2;
	}
	static const OUT& multiply(const OUT & arg1, bool arg2)
	{
		return arg1;
	}
	static void axpy(const OUT& a, bool x, OUT & y)
	{
		y = multiply(a, x);
	}

	static MPI_Op mpi_op()
	{
		static MPI_Op mpiop;
		static bool exists = false;
		if (exists)
			return mpiop;
		else
		{
			MPI_Op_create(MPI_func, true, &mpiop);
			exists = true;
			return mpiop;
		}
	}

	static void MPI_func(void * invec, void * inoutvec, int * len, MPI_Datatype *datatype)
	{
		if (*len > 0)
		{
			cout << "MPI Add should not happen (BoolCopy1stSRing)!" << endl;
			std::exit(1);
		}
	}
};



template <class T1, class T2, class OUT>
struct Select2ndSRing
{
	static OUT id() { return OUT(); }
	static bool returnedSAID() { return false; }
	static MPI_Op mpi_op() { return MPI_MAX; };
	static OUT add(const OUT & arg1, const OUT & arg2)
	{
		return arg2;
	}
	static OUT multiply(const T1 & arg1, const T2 & arg2)
	{
		// fragile since it wouldn't work with y <- x*A
		return static_cast<OUT>(arg2);
	}
	static void axpy(T1 a, const T2 & x, OUT & y)
	{
		//y = add(y, multiply(a, x));
		y = multiply(a, x);
	}
};

template <class T1, class T2>
struct SelectMaxSRing
{
	typedef typename promote_trait<T1,T2>::T_promote T_promote;
	static T_promote id() {  return -1; };
	static bool returnedSAID() { return false; }
	static MPI_Op mpi_op() { return MPI_MAX; };
	static T_promote add(const T_promote & arg1, const T_promote & arg2)
	{
		return std::max(arg1, arg2);
	}
	static T_promote multiply(const T1 & arg1, const T2 & arg2)
	{
		// we could have just returned arg2 but it would be
		// fragile since it wouldn't work with y <- x*A
		return (static_cast<T_promote>(arg1) * 
			static_cast<T_promote>(arg2) );
	}
	static void axpy(T1 a, const T2 & x, T_promote & y)
	{
		y = std::max(y, static_cast<T_promote>(a*x));
	}
};


// This one is used for BFS iteration
template <class T2>
struct SelectMaxSRing<bool, T2>
{
	typedef T2 T_promote;
	static T_promote id(){ return -1; };
	static bool returnedSAID() { return false; }
	static MPI_Op mpi_op() { return MPI_MAX; };
	static T_promote add(const T_promote & arg1, const T_promote & arg2)
	{
		return std::max(arg1, arg2);
	}
	static T_promote multiply(const bool & arg1, const T2 & arg2)
	{
		return arg2;
	}
	static void axpy(bool a, const T2 & x, T_promote & y)
	{
		y = std::max(y, x);
	}
};

template <class T1, class T2>
struct PlusTimesSRing
{
	typedef typename promote_trait<T1,T2>::T_promote T_promote;
	static T_promote id(){ return 0; }
	static bool returnedSAID() { return false; }
	static MPI_Op mpi_op() { return MPI_SUM; };
	static T_promote add(const T_promote & arg1, const T_promote & arg2)
	{
		return arg1+arg2;
	}
	static T_promote multiply(const T1 & arg1, const T2 & arg2)
	{
		return (static_cast<T_promote>(arg1) * 
			static_cast<T_promote>(arg2) );
	}
	static void axpy(T1 a, const T2 & x, T_promote & y)
	{
		y += a*x;
	}
};


template <class T1, class T2>
struct MinPlusSRing
{
	typedef typename promote_trait<T1,T2>::T_promote T_promote;
	static T_promote id() { return  std::numeric_limits<T_promote>::max(); };
	static bool returnedSAID() { return false; }
	static MPI_Op mpi_op() { return MPI_MIN; };
	static T_promote add(const T_promote & arg1, const T_promote & arg2)
	{
		return std::min(arg1, arg2);
	}
	static T_promote multiply(const T1 & arg1, const T2 & arg2)
	{
		return inf_plus< T_promote > 
		(static_cast<T_promote>(arg1), static_cast<T_promote>(arg2));
	}
	static void axpy(T1 a, const T2 & x, T_promote & y)
	{
		y = std::min(y, multiply(a, x));
	}
};


#endif
