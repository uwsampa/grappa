#ifdef __cplusplus
extern "C" {
#endif


#ifndef __NUMAUTIL_H__
#define __NUMAUTIL_H__


unsigned long numautil_cpuMaskForNode(int node);

unsigned int* numautil_cpusForNode(int node, int* cpuCount);

#endif


#ifdef __cplusplus
}
#endif
