
#include "walk.h"

// walk the list
uint64_t walk( node* bases[], uint64_t count, int num_refs, int start_index ) {
  uint64_t sum = 0;
  const int si = 0;
//printf("thread s\n");
  if (num_refs==1) {
  	node* i = bases[si];

  	while (count > 0) {
    		i = i->next;
   		 count--;
  	}
    	sum += (uint64_t) i; // do some work to make the optimizer happy

   } else if (num_refs==2) { 
	node* i0 = bases[si];
	node* i1 = bases[si+1];
   
	while (count > 0) {
		i0 = i0->next; i1 = i1->next;
		count--;
	} 
	sum += (uint64_t)i0 +  (uint64_t)i1;

   } else if (num_refs==3) {	
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];   

	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next;
		count--;
	}
 	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2;
   
   } else if (num_refs==4) {
	
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
		count--;
	} 
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3;
   
   } else if (num_refs==5) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4;
   
   } else if (num_refs==6) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; 
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5;

   } else if (num_refs==7) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; 
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6;
   
   } else if (num_refs==8) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7;

    } else if (num_refs==9) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
	node* i8 = bases[si+8];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8;

    } else if (num_refs==10) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
	node* i8 = bases[si+8];
	node* i9 = bases[si+9];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9;

    } else if (num_refs==11) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
	node* i8 = bases[si+8];
	node* i9 = bases[si+9];
	node* i10 = bases[si+10];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next; i10 = i10->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9 + (uint64_t)i10;

     } else if (num_refs==12) {
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
	node* i4 = bases[si+4];
	node* i5 = bases[si+5];
	node* i6 = bases[si+6];
	node* i7 = bases[si+7];
	node* i8 = bases[si+8];
	node* i9 = bases[si+9];
	node* i10 = bases[si+10];
	node* i11 = bases[si+11];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next; i10 = i10->next; i11 = i11->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9 + (uint64_t)i10 + (uint64_t)i11;
     } 
  return sum;
}


// walk the list
uint64_t multiwalk( node* bases[], 
		    uint64_t concurrent_reads, 
		    uint64_t count ) {
  node* i = bases[0];
  node* j = concurrent_reads >=  2 ? bases[1] : NULL;
  node* k = concurrent_reads >=  3 ? bases[2] : NULL;
  node* l = concurrent_reads >=  4 ? bases[3] : NULL;
  node* m = concurrent_reads >=  5 ? bases[4] : NULL;
  node* n = concurrent_reads >=  6 ? bases[5] : NULL;
  node* o = concurrent_reads >=  7 ? bases[6] : NULL;
  node* p = concurrent_reads >=  8 ? bases[7] : NULL;
  node* q = concurrent_reads >=  9 ? bases[8] : NULL;
  node* r = concurrent_reads >= 10 ? bases[9] : NULL;
  node* s = concurrent_reads >= 11 ? bases[10] : NULL;
  node* t = concurrent_reads >= 12 ? bases[11] : NULL;
  
  switch (concurrent_reads) {
  case 1: 
    {
      while (count > 0) {
	i = i->next;
	count--;
      }
    }
    break;

  case 2: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	count--;
      }
    }
    break;

  case 3: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	count--;
      }
    }
    break;

  case 4: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	count--;
      }
    }
    break;


  case 5: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	count--;
      }
    }
    break;


  case 6: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	count--;
      }
    }
    break;


  case 7: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	count--;
      }
    }
    break;

  case 8: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	count--;
      }
    }
    break;

  case 9: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	count--;
      }
    }
    break;

  case 10: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	r = r->next;
	count--;
      }
    }
    break;

  case 11: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	r = r->next;
	s = s->next;
	count--;
      }
    }
    break;

  case 12: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	r = r->next;
	s = s->next;
	t = t->next;
	count--;
      }
    }
    break;


    }

  return (uint64_t)i
    + (uint64_t)j
    + (uint64_t)k
    + (uint64_t)l
    + (uint64_t)m
    + (uint64_t)n
    + (uint64_t)o
    + (uint64_t)p
    + (uint64_t)q
    + (uint64_t)r
    + (uint64_t)s
    + (uint64_t)t;
}
