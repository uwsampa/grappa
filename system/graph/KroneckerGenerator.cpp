
struct mrg_state {
  uint_fast32_t z1, z2, z3, z4, z5;
  
  mrg_state(uint64_t userseed1, uint64_t userseed2) {
  	z1 = (userseed1 & 0x3FFFFFFF) + 1;
  	z2 = ((userseed1 >> 30) & 0x3FFFFFFF) + 1;
  	z3 = (userseed2 & 0x3FFFFFFF) + 1;
  	z4 = ((userseed2 >> 30) & 0x3FFFFFFF) + 1;
  	z5 = ((userseed2 >> 60) << 4) + (userseed1 >> 60) + 1;
  }
  
  void skip(uint_least64_t exponent_high, uint_least64_t exponent_middle, uint_least64_t exponent_low) {
    /* fprintf(stderr, "skip(%016" PRIXLEAST64 "%016" PRIXLEAST64 "%016" PRIXLEAST64 ")\n", exponent_high, exponent_middle, exponent_low); */
    int byte_index;
    for (byte_index = 0; exponent_low; ++byte_index, exponent_low >>= 8) {
      uint_least8_t val = (uint_least8_t)(exponent_low & 0xFF);
      if (val != 0) mrg_step(&mrg_skip_matrices[byte_index][val], state);
    }
    for (byte_index = 8; exponent_middle; ++byte_index, exponent_middle >>= 8) {
      uint_least8_t val = (uint_least8_t)(exponent_middle & 0xFF);
      if (val != 0) mrg_step(&mrg_skip_matrices[byte_index][val], state);
    }
    for (byte_index = 16; exponent_high; ++byte_index, exponent_high >>= 8) {
      uint_least8_t val = (uint_least8_t)(exponent_high & 0xFF);
      if (val != 0) mrg_step(&mrg_skip_matrices[byte_index][val], state);
    }
  }
  
protected:
  
  static void mrg_apply_transition(const mrg_transition_matrix* restrict mat, const mrg_state* restrict st, mrg_state* r) {
    uint_fast32_t o1 = mod_mac_y(mod_mul(mat->d, st->z1), mod_mac4(0, mat->s, st->z2, mat->a, st->z3, mat->b, st->z4, mat->c, st->z5));
    uint_fast32_t o2 = mod_mac_y(mod_mac2(0, mat->c, st->z1, mat->w, st->z2), mod_mac3(0, mat->s, st->z3, mat->a, st->z4, mat->b, st->z5));
    uint_fast32_t o3 = mod_mac_y(mod_mac3(0, mat->b, st->z1, mat->v, st->z2, mat->w, st->z3), mod_mac2(0, mat->s, st->z4, mat->a, st->z5));
    uint_fast32_t o4 = mod_mac_y(mod_mac4(0, mat->a, st->z1, mat->u, st->z2, mat->v, st->z3, mat->w, st->z4), mod_mul(mat->s, st->z5));
    uint_fast32_t o5 = mod_mac2(mod_mac3(0, mat->s, st->z1, mat->t, st->z2, mat->u, st->z3), mat->v, st->z4, mat->w, st->z5);
    r->z1 = o1;
    r->z2 = o2;
    r->z3 = o3;
    r->z4 = o4;
    r->z5 = o5;
  }
  
};


namespace Grappa {

  TupleGraph TupleGraph::generate_kronecker(int scale, int64_t desired_nedge, 
                                            uint64_t seed1, uint64_t seed2) {
    TupleGraph tg(desired_nedge);
    
    uint_fast32_t seed[5];
    make_mrg_seed(seed1, seed2, seed);
    
  	mrg_state state;
  	int64_t nverts = (int64_t)1 << logN;
  	int64_t ei;
	
  	mrg_seed(&state, seed);
	
  	uint64_t val0, val1; /* Values for scrambling */
  	{
  		mrg_state new_state = state;
  		mrg_skip(&new_state, 50, 7, 0);
  		val0 = mrg_get_uint_orig(&new_state);
  		val0 *= UINT64_C(0xFFFFFFFF);
  		val0 += mrg_get_uint_orig(&new_state);
  		val1 = mrg_get_uint_orig(&new_state);
  		val1 *= UINT64_C(0xFFFFFFFF);
  		val1 += mrg_get_uint_orig(&new_state);
  	}
    
    Grappa::forall(edges+start_edge, end_edge-start_edge,
      [nverts, logN, state, val0, val1](int64_t index, packed_edge& edge) {
        // copy the random generator state and seek
        mrg_state new_state = state;
        mrg_skip(&new_state, 0, index, 0);

        make_one_edge(nverts, 0, logN, &new_state, &edge, val0, val1);
      }
    );
    
  }
  
}