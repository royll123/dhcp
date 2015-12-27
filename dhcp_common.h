#ifndef DHCP_COMMON_T
#define DHCP_COMMON_T

#include <stdint.h>
#include <netinet/in.h>

#define DHCPDISCOVER    1
#define DHCPREQUEST     2
#define DHCPOFFER       3
#define DHCPPACK        4

#define DHCP_PORT		51230

struct dhcph {
    uint8_t type;
    uint8_t code;
    uint16_t ttl;
    in_addr_t address;
    uint32_t netmask;
};

#endif
