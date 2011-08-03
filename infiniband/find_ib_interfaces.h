
#ifndef __FIND_IB_INTERFACES_H__
#define __FIND_IB_INTERFACES_H__

#include <arpa/inet.h>

void print_interfaces();
//int find_ib_interfaces( struct in_addr addrs[], int max );
int find_ib_interfaces( struct sockaddr addrs[], int max );

#endif
