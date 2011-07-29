#include <stdio.h>
#include <stdlib.h>

#include "base.h"


void test_fetch_and_add() {
    uint64  v, r;
    
    v = 1;
    r = atomic_fetch_and_add_uint64(&v, 2);
    if (r != 1) {
        printf("atomic_fetch_and_add -- FAILED expected %d got %lld value = %lld\n",
            1, r, v);
        exit(1);
        }
    address_expose();
    if (v != 3) {
        printf("atomic_fetch_and_add -- FAILED memory is %lld not %d, result = %lld\n",
            v, 3, r);
        exit(2);
        }
    printf("atomic_fetch_and_add -- SUCCESS\n");
    }

void test_compare_and_set() {
    uint64  v, r;
    
    v = 1;
    r = atomic_cmpset_uint64(&v, 1, 2);
    if (v != 2 || r != 1) {
        printf("atomic_cmpset -- FAILED on match case.  v = %lld (expected 2) r= %lld (expected 1)\n",
            v, r);
        exit(3);
        }
    r = atomic_cmpset_uint64(&v, 1, 3);
    if (v != 2 || r != 2) {
        printf("atomic_cmpset -- FAILED on non-match case.  v = %lld (expected 2) r= %lld (expected 2)\n",
            v, r);
        exit(3);
        }
    printf("atomic_cmpset -- SUCCESS\n");        
}
    
int main(int argc, char *argv[]) {
    test_fetch_and_add();
    test_compare_and_set();
    return 0;
    }
    
    