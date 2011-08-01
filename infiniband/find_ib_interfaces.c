#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>

#include <sys/ioctl.h>
#include <net/if.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "find_ib_interfaces.h"

void print_interfaces() {

  // socket
  int sck = socket( AF_INET, SOCK_DGRAM, 0 );
  if (sck < 0) { 
    perror("socket"); 
    exit(1); 
  }

  // query
  char if_buf[BUFSIZ];
  struct ifconf ifc;
  
  ifc.ifc_len = sizeof( if_buf );
  ifc.ifc_buf = if_buf;
  if ( ioctl( sck, SIOCGIFCONF, &ifc ) < 0 ) {
    perror("ioctl(SIOCGIFCONF)");
    exit(1);
  }

  // iterate
  struct ifreq * ifr;
  ifr = ifc.ifc_req;
  int num_interfaces = ifc.ifc_len / sizeof( struct ifreq );
  int interface_num ;
  for( interface_num = 0; interface_num < num_interfaces; ++interface_num ) {
    struct ifreq * interface = &ifr[interface_num];

    // get IP
    printf("interface %s: IP %s\n", interface->ifr_name,
	   inet_ntoa(((struct sockaddr_in *)&interface->ifr_addr)->sin_addr));
  }

}

int find_ib_interfaces( struct sockaddr addrs[], int max ) {
  int count = 0;

  // socket
  int sck = socket( AF_INET, SOCK_DGRAM, 0 );
  if (sck < 0) { 
    perror("socket"); 
    exit(1); 
  }

  // query
  char if_buf[BUFSIZ];
  struct ifconf ifc;
  
  ifc.ifc_len = sizeof( if_buf );
  ifc.ifc_buf = if_buf;
  if ( ioctl( sck, SIOCGIFCONF, &ifc ) < 0 ) {
    perror("ioctl(SIOCGIFCONF)");
    exit(1);
  }

  // iterate
  struct ifreq * ifr;
  ifr = ifc.ifc_req;
  int num_interfaces = ifc.ifc_len / sizeof( struct ifreq );
  int interface_num;
  for( interface_num = 0; interface_num < num_interfaces; ++interface_num ) {

    // simple heuristic to detect infiniband interface
    int match = ( ('\0' != ifr[interface_num].ifr_name[0]) && 
		  ('i'  == ifr[interface_num].ifr_name[0]) &&
		  ('\0' != ifr[interface_num].ifr_name[1]) && 
		  ('b'  == ifr[interface_num].ifr_name[1]) );
    if ( 0 != match && count < max ) {
      addrs[count] = ifr[interface_num].ifr_addr;
      ++count;
    }
    
  }

  return count;
}
