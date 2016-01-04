#ifndef DHCP_COMMON_T
#define DHCP_COMMON_T

#include <stdint.h>
#include <netinet/in.h>

#define DHCPDISCOVER    1
#define DHCPREQUEST     2
#define DHCPOFFER       3
#define DHCPACK			4
#define DHCPRELEASE		5

#define DHCP_CODE_OK		0
#define DHCP_CODE_REQ_ALC	10
#define DHCP_CODE_REQ_EXT	11
#define DHCP_CODE_ERR_NONE	129
#define DHCP_CODE_ERR_OVL	130

#define DHCP_PORT		51230

struct dhcph {
    uint8_t type;
    uint8_t code;
    uint16_t ttl;
    in_addr_t address;
    uint32_t netmask;
};

void print_dhcp_header(struct dhcph*);
#endif
