#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "queue.h"

static struct address address_list;
static struct queue q;
static int address_freeze;

void queue_init()
{
	address_list.fp = &address_list;
	address_list.bp = &address_list;
	q.first = NULL;
	q.last = NULL;
	address_freeze = 0;
}

void freeze_address()
{
	address_freeze = 1;
}


struct address* find_address(struct in_addr ip, struct in_addr netmask)
{
	struct address* p;
	for(p = address_list.fp; p != &address_list; p = p->fp){
        if(p->ip.s_addr == ip.s_addr && p->netmask.s_addr == netmask.s_addr){
            break;
        }
    }
    if(p != &address_list){
		return p;
	} else {
		return NULL;
	}
}

struct address* get_address(struct in_addr ip, struct in_addr netmask)
{
	struct address* p = find_address(ip, netmask);
	if(p != NULL) return p;
	
	if(address_freeze == 0){
		p = (struct address*)malloc(sizeof(struct address));
		if(p == NULL){
			fprintf(stderr, "cannot allocate memory.\n");
			exit(1);
		}

		p->ip = ip;
		p->netmask = netmask;
		p->fp = &address_list;
		p->bp = address_list.bp;
		address_list.bp->fp = p;
		address_list.bp = p;
	} else {
		p = NULL;
	}
	return p;
}

int queue_push(struct in_addr ip, struct in_addr netmask)
{
	struct address** p;
	if(q.last == NULL){
		p = &q.last;
	} else {
		p = &q.last->q_next;
	}
	
	*p = get_address(ip, netmask);
	if(*p == NULL) return -1;

	(*p)->q_next = NULL;
	q.last = *p;
	if(q.first == NULL){
		q.first = *p;
	}
	return 0;
}

int queue_pop(struct in_addr *ip, struct in_addr *netmask)
{
	struct address* p = q.first;
	if(p == NULL){
		return -1;
	}
	*ip = p->ip;
	*netmask = p->netmask;

	q.first = p->q_next;
	if(q.last == p){
		q.last = NULL;
	}
	return 0;
}

void queue_end()
{
	struct address *p = address_list.fp;

	while(p != &address_list){
		p = p->fp;
		free(p->bp);
	}
}

void debug_print()
{
	struct address* p = q.first;
	fprintf(stderr, "IP addr queue data start\n");
	while(p != NULL){
		fprintf(stderr, "ip: %s ", inet_ntoa(p->ip));
		fprintf(stderr, "mask: %s\n", inet_ntoa(p->netmask));
		p = p->q_next;
	}
	fprintf(stderr, "IP addr queue data end\n");
}
