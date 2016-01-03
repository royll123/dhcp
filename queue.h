#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#include <netinet/in.h>

struct address {
    struct in_addr ip;
    uint32_t netmask;
	struct address* fp;
	struct address* bp;
	struct address* q_next;
};

struct queue {
    struct address* first;
    struct address* last;
};

void queue_init();
void freeze_address();
struct address* find_address(struct in_addr ip, uint32_t netmask);
int queue_push(struct in_addr ip, uint32_t netmask);
int queue_pop(struct in_addr *ip, uint32_t *netmask);
void queue_end();
void debug_print();
#endif
