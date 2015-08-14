#pragma once
#include <Grappa.hpp>
#include <GlobalCompletionEvent.hpp>

#include "radish_utils.h"
#include "stats.h"
#include "strings.h"
#include "relation.hpp"
#include "DHT_symmetric.hpp"
#include "DoubleDHT.hpp"
#include <vector>


template <typename P>
class Operator {
  public:
    virtual bool next(P& outTuple) = 0;
    virtual void close() = 0;
};

template <typename C, typename P>
class BasePipelined : public Operator<P> {
  protected:
    Operator<C> * input; // operator produces what we consume
  public:
    BasePipelined(Operator<C>* input) : input(input) { }

    void close() {
      this->input->close();
    }
};

template<typename C, typename P>
class Apply : public BasePipelined<C,P>{
  public:
    using BasePipelined<C,P>::BasePipelined;
  bool next(P& t_000) {
    C t_004;
    if (this->input->next(t_004)) {
      apply(t_000, t_004);
      return true;
    } else {
      return false;
    }
  }

  protected:
  // subclass is generated and implements
  // this method
  virtual void apply(P& p, C& c) = 0; 
};

template <typename C>
class Store : public BasePipelined<C,int> {
  private:
    std::vector<C> * _res;
  public:
    Store(Operator<C>* input, std::vector<C> * res)
      : BasePipelined<C,int>(input)
      , _res(res) { }
  bool next(int& ignore) {
    C t_000; 
    if (this->input->next(t_000)) {
      _res->push_back(t_000);
      VLOG(2) << t_000;
      return true;
    } else {
      return false;
    }
  }
};

template <typename C, typename K, typename V, GlobalCompletionEvent * GCE, V (*UpF)(const V& oldval, const C& incVal), V (*Init)(void) >
class AggregateSink : public BasePipelined<C,int> {
  private:
    typedef hash_tuple::hash<K> Hash;
  
  protected:
    GlobalAddress<DHT_symmetric<K, V, Hash>> group_hash;

  public:
    AggregateSink(Operator<C>* input, GlobalAddress<
                      DHT_symmetric<K, V, Hash>> group_hash_000)
   : BasePipelined<C,int>(input)
   , group_hash(group_hash_000) { }

    bool next(int& ignore) {
      C t_002;
      if (this->input->next(t_002)) {
        VLOG(4) << "update with tuple " << t_002;
        group_hash->template update<GCE, C, UpF, Init>(mktuple(t_002), t_002);
        return true;
      } else {
        return false;
      }
    }

  protected:
  // subclass is generated and implements
  // this method
  virtual K mktuple(C& val) = 0; 
};

template <typename C, typename P, typename K>
class AggregateSource : public Operator<P> {
  private:
    typedef C  V;
    typedef hash_tuple::hash<K> Hash;

    GlobalAddress<DHT_symmetric<K, V, Hash>> group_hash;
    
typedef decltype(group_hash->get_local_map()->begin()) iter_type;
    iter_type iter;
  protected:
typedef typename std::iterator_traits<iter_type>::value_type map_output_t;

  // subclass is generated and implements
  // this method
  virtual void mktuple(P& out, map_output_t& inp) = 0;
  public:
    AggregateSource(
                   GlobalAddress<
                      DHT_symmetric<K, V, Hash>> group_hash_000) {
      group_hash = group_hash_000;
      iter = group_hash->get_local_map()->begin();
      VLOG(3) << "local size: " << group_hash->get_local_map()->size();
    }

  bool next(P& t_010) {//P=MaterializedTupleRef_V8_10 
    if (iter != group_hash->get_local_map()->end()) {
      VLOG(3) << "got a tuple";
      auto V6 = *(this->iter);
      this->mktuple(t_010, V6);
      ++iter;
      return true;
    } else {
      return false;
    }
  }

  void close() {
  }
};


template <typename P>
class Scan : public Operator<P> {
  private:
    uint64_t index;
    uint64_t size;
    Relation<aligned_vector<P>> rel; 
  public:
    Scan(Relation<aligned_vector<P>> rel) {
      this->index = 0;
      this->rel = rel;
      this->size = rel.data->vector.size();
    }
  bool next(P& t_003) {
    VLOG(3) << "index(" << index << ") <? size(" << size << ")";
    if (index < size) {
      t_003 = rel.data->vector[index++];
      VLOG(3) << t_003;
      return true;
    } else {
      return false;
    }
  }

  void close() {
  }
};


template <typename C, typename P>
class Select : public BasePipelined<C, P> {
  public:
    using BasePipelined<C,P>::BasePipelined;
    bool next(P& t_100) {
      C t_003;
      while (this->input->next(t_003)) {
        if (predicate(t_003)) {
          t_100 = t_003;
          return true;
        }
      }
      return false;
    }

  protected:
  // subclass is generated and implements
  // this method
  virtual bool predicate(C& t) = 0;

};

template <typename K, typename CL, typename CR, typename Hash>
class HashJoinSinkLeft : public BasePipelined<CL,int> {
  public:
    typedef DoubleDHT<K, CL, CR, Hash> dht_t;
    dht_t * double_hash;

    HashJoinSinkLeft(dht_t * hash_000, Operator<CL> * left)
      : BasePipelined<CL,int>(left)
      , double_hash(hash_000) { }

    bool next(int& ignore) {
      CL t_000;
      if (this->input->next(t_000)) {
        VLOG(3) << t_000;
        double_hash->insert_left(mktuple(t_000), t_000);
        return true;
      } else {
        return false;
      }
    }

  protected:
  virtual K mktuple(CL& t) = 0;
};

template <typename K, typename CL, typename CR, typename Hash>
class HashJoinSinkRight : public BasePipelined<CR,int> {
  public:
    typedef DoubleDHT<K, CL, CR, Hash> dht_t;
    dht_t * double_hash;

    HashJoinSinkRight(dht_t * hash_000, Operator<CR>* right)
      : BasePipelined<CR,int>(right)
      , double_hash(hash_000) { }

    bool next(int& ignore) {
      CR t_000;
      if (this->input->next(t_000)) {
        VLOG(3) << t_000;
        double_hash->insert_right(mktuple(t_000), t_000);
        return true;
      } else {
        return false;
      }
    }

  protected:
  virtual K mktuple(CR& t) = 0;
};

template <typename K, typename CL, typename CR, typename Hash, typename P>
class HashJoinSource : public Operator<P> {
  public:
    typedef DoubleDHT<K, CL, CR, Hash> dht_t;
  private:
    dht_t * double_hash;
    Grappa::FullEmpty<std::pair<bool, std::pair<CL, CR>>> * match_iter;

  public:

    HashJoinSource(dht_t * hash_000)
      : double_hash(hash_000)
      , match_iter(NULL) { }

    bool next(P& t) {
      // init
      if (match_iter == NULL) { match_iter = double_hash->matches(); }
      
      auto entry = this->match_iter->readFE();
      if (entry.first) {
        join_coarse_result_count++;
        t = this->mktuple(entry.second.first, entry.second.second);
        return true;
      } else {
        return false;
      }
    }

    void close() {
    }
  
  protected:
    virtual P mktuple(CL& tl, CR& tr) = 0;
};

using namespace Grappa;
void iterate(Operator<int> ** fragment, GlobalCompletionEvent * gce) {
  auto origin = mycore();
  gce->enroll(cores());

  on_all_cores([=] {
      int dummy;
      auto fp = *fragment;
      while (fp->next(dummy));
      fp->close();
      gce->send_completion(origin);
      gce->wait();
  });
}
