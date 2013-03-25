#ifndef VEC_ITERATOR_H
#define VEC_ITERATOR_H

#include "FullyDistVec.h"
#include "FullyDistSpVec.h"

template <class IT, class NT>
class VectorLocalIterator
{
	public:
	virtual ~VectorLocalIterator() {}
	
	virtual IT LocalToGlobal(IT loc_idx) const = 0;
	virtual IT GlobalToLocal(IT gbl_idx) const = 0;
	
	virtual bool Next() = 0;
	virtual bool NextTo(IT loc_idx) = 0;
	virtual bool HasNext() = 0;
	virtual IT GetLocIndex() const = 0;
	virtual NT& GetValue() const = 0;

	virtual void Del() = 0;
	
	virtual void Set(const IT loc_idx, const NT& val) = 0;
};

template <class IT, class NT>
class DenseVectorLocalIterator: public VectorLocalIterator<IT, NT>
{
	protected:
	FullyDistVec<IT, NT>& v;
	IT iter_idx;
	
	public:
	DenseVectorLocalIterator(FullyDistVec<IT, NT>& in_v): v(in_v), iter_idx(0) {}
	
	IT LocalToGlobal(IT loc_idx) const
	{
		return v.LengthUntil() + loc_idx;
	}
	
	IT GlobalToLocal(IT gbl_idx) const
	{
		IT ret;
		v.Owner(gbl_idx, ret);
		return ret;
	}
	
	
	bool Next()
	{
		iter_idx++;
		bool exists = ((unsigned)iter_idx < v.arr.size());
		if (!exists)
			iter_idx = -1;
		return exists;
	}

	bool NextTo(IT loc_idx)
	{
		iter_idx = loc_idx;
		return iter_idx > 0 && (unsigned)iter_idx < v.arr.size();
	}

	bool HasNext()
	{
		return iter_idx >= 0 && (unsigned)iter_idx < v.arr.size();
	}
	
	IT GetLocIndex() const
	{
		if ((unsigned)iter_idx < v.arr.size())
			return iter_idx;
		else
			return -1;
	}
	
	NT& GetValue() const
	{
		return v.arr[iter_idx];
	}

	void Del()
	{
		assert(false);
	}
	
	void Set(const IT loc_idx, const NT& val)
	{
		v.arr[loc_idx] = val;
	}
};

template <class IT, class NT>
class SparseVectorLocalIterator: public VectorLocalIterator<IT, NT>
{
	protected:
	FullyDistSpVec<IT, NT>& v;
	IT iter_idx;
	
	public:
	SparseVectorLocalIterator(FullyDistSpVec<IT, NT>& in_v): v(in_v), iter_idx(0)
	{
		if (v.ind.size() == 0)
			iter_idx = -1;
	}
	
	IT LocalToGlobal(IT loc_idx) const
	{
		return v.LengthUntil() + loc_idx;
	}
	
	IT GlobalToLocal(IT gbl_idx) const
	{
		IT ret;
		v.Owner(gbl_idx, ret);
		return ret;
	}
	
	bool Next()
	{
		iter_idx++;
		bool exists = ((unsigned)iter_idx < v.ind.size());
		if (!exists)
			iter_idx = -1;
		return exists;
	}

	bool NextTo(IT loc_idx)
	{
		typename vector<IT>::iterator iter = lower_bound(v.ind.begin()+iter_idx, v.ind.end(), loc_idx);	
		if(iter == v.ind.end())	// beyond limits, insert from back
		{
			iter_idx = -1;
			return false;
		}
		else if (loc_idx < *iter)	// not found, but almost
		{
			iter_idx = iter - v.ind.begin();
			return false;
		}
		else // found
		{
			iter_idx = iter - v.ind.begin();
			return true;
		}
	}
	
	bool HasNext()
	{
		return iter_idx >= 0 && (unsigned)iter_idx < v.ind.size();
	}
	
	IT GetLocIndex() const
	{
		if (iter_idx < 0)
			return -1;
		else
			return v.ind[iter_idx];
	}
	
	NT& GetValue() const
	{
		return v.num[iter_idx];
	}

	void Del()
	{
		v.ind.erase(v.ind.begin()+iter_idx);
		v.num.erase(v.num.begin()+iter_idx);
		if ((unsigned)iter_idx >= v.ind.size())
			iter_idx = -1;
	}
	
	void Set(const IT loc_idx, const NT& val)
	{
		// see if we're just replacing the current value
		/*if (loc_idx >= 0 && loc_idx == v.ind[iter_idx])
		{
			v.num[iter_idx] = val;
			return;
		}*/
		
		// inserted elsewhere
		// This is from FullyDistSpVec::SetElement():
		typename vector<IT>::iterator iter = lower_bound(v.ind.begin(), v.ind.end(), loc_idx);	
		if(iter == v.ind.end())	// beyond limits, insert from back
		{
			v.ind.push_back(loc_idx);
			v.num.push_back(val);
		}
		else if (loc_idx < *iter)	// not found, insert in the middle
		{
			// the order of insertions is crucial
			// if we first insert to ind, then ind.begin() is invalidated !
			v.num.insert(v.num.begin() + (iter-v.ind.begin()), val);
			v.ind.insert(iter, loc_idx);
		}
		else // found
		{
			*(v.num.begin() + (iter-v.ind.begin())) = val;
		}
	}
	
	void Append(const IT loc_idx, const NT& val)
	{
		v.ind.push_back(loc_idx);
		v.num.push_back(val);
	}
};

#include "VecIterator.cpp"
#endif
