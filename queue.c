#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "queue.h"

static struct queue q;
static int queue_max;

void queue_init()
{
	q.first = NULL;
	q.last = NULL;
}

int queue_push(struct in_addr ip, uint32_t netmask)
{
	struct address** p;
	if(q.last == NULL){
		p = &q.last;
	} else {
		fprintf(stderr, "use last\n");
		p = &q.last->next;
	}
	*p = (struct address*)malloc(sizeof(struct address));
	if(p == NULL){
		fprintf(stderr, "cannot allocate memory.\n");
		exit(1);
	}
	(*p)->ip = ip;
	(*p)->netmask = netmask;
	(*p)->next = NULL;
	q.last = *p;
	if(q.first == NULL){
		q.first = *p;
	}
	return 0;
}

int queue_pop(struct in_addr *ip, uint32_t *netmask)
{
	struct address* p = q.first;
	if(p == NULL){
		return -1;
	}
	*ip = p->ip;
	*netmask = p->netmask;

	q.first = p->next;
	if(q.last == p){
		q.last = NULL;
	}
	free(p);
	return 0;
}

void debug_print()
{
	struct address* p = q.first;
	fprintf(stderr, "IP addr queue data start\n");
	while(p != NULL){
		fprintf(stderr, "ip: %s ", inet_ntoa(p->ip));
		fprintf(stderr, "mask: %d\n", p->netmask);
		p = p->next;
	}
	fprintf(stderr, "IP addr queue data end\n");
}
