#pragma once
#include <Grappa.hpp>


template <typename T>
class TemporaryTable {
  /* A temporary table that is double buffered
   * for use in loops
   */

  private:
    Relation<aligned_vector<T>> _src;
    Relation<aligned_vector<T>> _sink;
    uint64_t producers;
    
    void close_round() {
      /* precondition: "this" must be a globally valid pointer; e.g.,
          it must point to a global variable 
       */

      on_all_cores([=] {
          auto temp = this->_sink.data;
          this->_sink.data = this->_src.data;
          this->_src.data = temp;
          this->_sink.data->vector.clear();
      });
    }

  public:
    TemporaryTable()
      : _src()
      , _sink()
      , producers(0) { }


    void init() {
      /* precondition: "this" must be a globally valid pointer; e.g.,
          it must point to a global variable 
       */

      decltype(_src) l_src;
      l_src.data = Grappa::symmetric_global_alloc<aligned_vector<T>>();
      l_src.numtuples = 0;

      decltype(_sink) l_sink;
      l_sink.data = Grappa::symmetric_global_alloc<aligned_vector<T>>();
      l_sink.numtuples = 0;

      on_all_cores([=] {
        this->_src = l_src;
        this->_sink = l_sink;
      });
    }

    void append(T& t) {
      // a partition-local operation

      _sink.data->vector.push_back(t); 
    }
     
    GlobalAddress<aligned_vector<T>> read() {
      // a partition-local operation

      return _src.data;
    }

    void register_producers(uint64_t n) {
      CHECK( producers == 0 );
      CHECK( n > 0 );
      producers = n;
    }

    void release_producer() {
      CHECK( producers > 0 );
      producers -= 1;
      if (producers == 0) {
        close_round();
      }
    } 

    std::ostream& dump(std::ostream& o) const {
      auto src_size = _src.data->vector.size();
      auto sink_size = _sink.data->vector.size();
      o << "src: [";
      if (src_size > 0) {
        o << _src.data->vector[0] << "...(size: " << src_size << ")";
      } else {
        o << "empty";
      }
      o << "]";

      o << "sink: [";
      if (sink_size > 0) {
        o << _sink.data->vector[0] << "...(size: " << sink_size << ")";
      } else {
        o << "empty";
      }
      o << "]";
      return o;
    }

};

template <typename T>
std::ostream& operator<< (std::ostream& o, const TemporaryTable<T>& t) {
  return t.dump(o);
}

