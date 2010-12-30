#include "greenery/thread.h"
#include "linked_list-walk.h"

inline void prefetch(void *addr) {
  __builtin_prefetch(addr,0,0);
}

// walk the list
uint64_t walk_prefetch( thread* me, node* bases[], uint64_t count, int num_refs, int start_index ) {
  uint64_t sum = 0;
  const int si = 0;
//printf("thread s\n");
  if (num_refs==1) {
  	node* i = bases[si];

  	while (count > 0) {
   		count--;
		prefetch(&(i->next));
		thread_yield(me);
    		i = i->next;
  	}
    	sum += (uint64_t) i; // do some work to make the optimizer happy

   } else if (num_refs==2) { 
	node* i0 = bases[si];
	node* i1 = bases[si+1];
   
	while (count > 0) {
		count--;
		prefetch(&(i0->next));
		prefetch(&(i1->next));
		thread_yield(me);
		i0 = i0->next; i1 = i1->next;
	} 
	sum += (uint64_t)i0 +  (uint64_t)i1;

   } else if (num_refs==3) {	
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];   

	while (count > 0) {
		count--;
		prefetch(&(i0->next));
		prefetch(&(i1->next));
		prefetch(&(i2->next));
		thread_yield(me);
		i0 = i0->next; i1 = i1->next; i2 = i2->next;
	}
 	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2;
   
   } else if (num_refs==4) {
	
	node* i0 = bases[si];
	node* i1 = bases[si+1];
	node* i2 = bases[si+2];
	node* i3 = bases[si+3];  
 
	while (count > 0) {
		count--;
		prefetch(&(i0->next));
		prefetch(&(i1->next));
		prefetch(&(i2->next));
		prefetch(&(i3->next));
		thread_yield(me);	
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
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


// walk the list without prefetching
uint64_t walk_raw( thread* me, node* bases[], uint64_t count, int num_refs, int start_index ) {
  uint64_t sum = 0;
  const int si = 0;
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
     } else if (num_refs==13) {
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
	node* i12 = bases[si+12];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next; i10 = i10->next; i11 = i11->next;
		i12 = i12->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9 + (uint64_t)i10 + (uint64_t)i11 + (uint64_t)i12;
     }else if (num_refs==14) {
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
	node* i12 = bases[si+12];
	node* i13 = bases[si+13];
 
	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next; i10 = i10->next; i11 = i11->next;
		i12 = i12->next; i13 = i13->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9 + (uint64_t)i10 + (uint64_t)i11 + (uint64_t)i12
			+ (uint64_t)i13;
     }else if (num_refs==15) {
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
	node* i12 = bases[si+12];
	node* i13 = bases[si+13];
	node* i14 = bases[si+14];
	 

	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next; i10 = i10->next; i11 = i11->next;
		i12 = i12->next; i13 = i13->next; i14 = i14->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9 + (uint64_t)i10 + (uint64_t)i11 + (uint64_t)i12
			+ (uint64_t)i13 + (uint64_t)i14;
     }else if (num_refs==16) {
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
	node* i12 = bases[si+12];
	node* i13 = bases[si+13];
	node* i14 = bases[si+14];
	node* i15 = bases[si+15]; 

	while (count > 0) {
		i0 = i0->next; i1 = i1->next; i2 = i2->next; i3 = i3->next;
                i4 = i4->next; i5 = i5->next; i6 = i6->next; i7 = i7->next;
		i8 = i8->next; i9 = i9->next; i10 = i10->next; i11 = i11->next;
		i12 = i12->next; i13 = i13->next; i14 = i14->next; i15 = i15->next;
		count--;
	}
	sum += (uint64_t)i0 +  (uint64_t)i1 +  (uint64_t)i2 +  (uint64_t)i3
                        + (uint64_t)i4 + (uint64_t)i5 + (uint64_t)i6 + (uint64_t)i7
			+ (uint64_t)i8 + (uint64_t)i9 + (uint64_t)i10 + (uint64_t)i11 + (uint64_t)i12
			+ (uint64_t)i13 + (uint64_t)i14 + (uint64_t)i15;
     }
  return sum;
}

