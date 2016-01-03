#include <stdio.h>
#include <arpa/inet.h>
#include "dhcp_common.h"

void print_dhcp_header(struct dhcph *h)
{
	struct in_addr ip = { h->address };
	
	fprintf(stderr, "*********DHCP Header start*********\n");
	fprintf(stderr, "Type: %d\n", h->type);
	fprintf(stderr, "Code: %d\n", h->code);
	fprintf(stderr, "TTL: %d\n", h->ttl);
	fprintf(stderr, "IP: %s\n", inet_ntoa(ip));
	fprintf(stderr, "Mask: %d\n", h->netmask);
	fprintf(stderr, "********* DHCP Header end *********\n");
}
