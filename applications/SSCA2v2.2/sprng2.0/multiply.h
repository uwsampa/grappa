#ifdef LONG64
#define mult_48_64(a,b,c)  c = (a*b); 

#define multiply(genptr) mult_48_64(genptr->seed,genptr->multiplier,genptr->seed); genptr->seed +=  genptr->prime; genptr->seed &= LSB48;

#else
#define mult_48_32(a,b,c)  c[0] = a[0]*b[0]; c[1] = a[1]*b[0]+a[0]*b[1];\
  c[2] = a[0]*b[2]+a[1]*b[1]+a[2]*b[0];\
  c[3] = a[3]*b[0]+a[2]*b[1]+a[1]*b[2]+a[0]*b[3]; 

#define multiply(genptr,m,s,res) s[3] = (unsigned int) genptr->seed[0] >> 12;\
    s[2] = genptr->seed[0] & 4095; s[1] = genptr->seed[1]  >> 12;\
    s[0] = genptr->seed[1] & 4095;\
    mult_48_32(m,s,res);\
    genptr->seed[1] = res[0] + ((res[1]&4095) << 12) + genptr->prime;\
    genptr->seed[0] = ( (unsigned int) genptr->seed[1] >> 24)\
      + res[2] + ((unsigned int) res[1] >> 12 ) + (res[3] << 12);\
    genptr->seed[1] &= 16777215; genptr->seed[0] &= 16777215;
#endif 
