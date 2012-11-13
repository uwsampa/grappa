
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <limits>

#include <ForkJoin.hpp>

typedef boost::mt19937_64 engine_t;
extern engine_t engine;

void init_engine() {
  engine = engine_t(12345L*Grappa_mynode());
}

template<typename T>
class FillRandom {
  private:
    FillRandom() {}
  public:
    typedef boost::uniform_int<T> dist_t;
    typedef boost::variate_generator<engine_t&,dist_t> gen_t;
    
    static gen_t gen; = gen_t(engine, dist_t(0, maxkey));
};

template<typename T>
inline void set_random(T * v) {

  // continue using same generator with multiple calls (to not repeat numbers)
  // but start at different seed on each node so we don't get overlap
  
  *v = FillRandom<T>::gen();
}

inline void fill_random(GlobalAddress<T> array, int64_t nelem) {
  forall_local< T, set_random<T> >(array, nelem);
}
