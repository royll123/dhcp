#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#include <netinet/in.h>

struct address {
    struct in_addr ip;
    uint32_t netmask;
	struct address* next;
};

struct queue {
    struct address* first;
    struct address* last;
};

void queue_init();
int queue_push(struct in_addr ip, uint32_t netmask);
int queue_pop(struct in_addr *ip, uint32_t *netmask);
void debug_print();
#endif
