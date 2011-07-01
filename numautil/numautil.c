#include <numa.h>
#include <sched.h>
#include <assert.h>

unsigned long numautil_cpuMaskForNode(int node) {
	struct bitmask* mask = numa_bitmask_alloc(numa_num_configured_cpus());
    assert(numa_bitmask_nbytes(mask) <= sizeof(unsigned long)); // ensure bitmask fits in 64 bits
	numa_node_to_cpus(node, mask);
    unsigned long r = *(mask->maskp);
    numa_bitmask_free(mask);
    return r;
}

unsigned int* numautil_cpusForNode(int node, int* cpuCount) {
    unsigned long mask = numautil_cpuMaskForNode(node);
    int numCpu = 0;
    for (int i=0; i<sizeof(unsigned long)*8; i++) {
        numCpu += (mask & (1L<<i)) ? 1 : 0;
    }

    unsigned int* result = (unsigned int*) malloc(sizeof(unsigned int)*numCpu);
    int index=0;
    for (int i=0; i<sizeof(unsigned long)*8; i++) {
        if (mask & (1L<<i)) {
            result[index++] = i;
        }
    }

    *cpuCount = numCpu;
    return result;
}

/*int main() {
    int cnt;
	 unsigned int* x = numautil_cpusForNode(0, &cnt);

    for (int i=0; i<cnt; i++) {
        printf("%u ", x[i]);
    }
    printf("\n(%d)\n", cnt);
}*/
