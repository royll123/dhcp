#include <stdio.h>
#include <arpa/inet.h>
#include "dhcp_common.h"

void print_dhcp_header(struct dhcph *h)
{
	struct in_addr ip = { h->address };
	struct in_addr mask = { h->netmask };
	printf("*********DHCP Header start*********\n");
	printf("Type: %d\n", h->type);
	printf("Code: %d\n", h->code);
	printf("TTL: %d\n", h->ttl);
	printf("IP: %s\n", inet_ntoa(ip));
	printf("Mask: %s\n", inet_ntoa(mask));
	printf("********* DHCP Header end *********\n");
}
