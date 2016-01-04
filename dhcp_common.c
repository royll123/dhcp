#include <stdio.h>
#include <arpa/inet.h>
#include "dhcp_common.h"

void print_dhcp_header(struct dhcph *h)
{
	struct in_addr ip = { h->address };
	
	printf("*********DHCP Header start*********\n");
	printf("Type: %d\n", h->type);
	printf("Code: %d\n", h->code);
	printf("TTL: %d\n", h->ttl);
	printf("IP: %s\n", inet_ntoa(ip));
	printf("Mask: %d\n", h->netmask);
	printf("********* DHCP Header end *********\n");
}
