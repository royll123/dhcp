#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dhcp_common.h"
#include "queue.h"

#define STAT_WAIT_DISCOVER  1
#define STAT_WAIT_REQUEST   2
#define STAT_WAIT_RELEASE   3


struct client {
    struct client *fp; /* 双方向リスト用ポインタ */
    struct client *bp; /* 双方向リスト用ポインタ */
    struct client *tout_fp; /* タイムアウト管理用ポインタ */
    struct client *tout_bp; /* タイムアウト管理用ポインタ */
    int start_time; /* 使用開始時刻 */
    int exp_time; /* タイムリミット */
    short stat; /* クライアントの状態 */
    struct in_addr cli_id; /* クライアントの識別子 (IP アドレス) */
    uint16_t cli_port; /* クライアントのポート番号 */
    struct in_addr alloc_addr; /* クライアントに割り当てた IP アドレス */
    uint32_t netmask; /* netmask */
};
struct client client_list; /* クライアントリストのリストヘッド */

struct client* get_client(struct in_addr*);
struct client* create_client();
void print_client(struct client*);
void read_config(char*);

int main(int argc, char* argv[])
{
    int s, msgtype, count;
    char buf[512];
	struct dhcph head;
    in_port_t myport = DHCP_PORT;
    struct client *cli;
    struct sockaddr_in myskt;
    struct sockaddr_in skt;
    socklen_t sktlen = sizeof(skt);
	

	// arguments
	if(argc != 2){
		fprintf(stderr, "Usage: server <config_file>\n");
		exit(1);
	}

	read_config(argv[1]);

    if((s = socket(PF_INET, SOCK_DGRAM, 0)) == -1){
        perror("socket");
        exit(1);
    }
    bzero(&myskt, sizeof(myskt));
    myskt.sin_family = AF_INET;
    myskt.sin_port = htons(myport);
    myskt.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(s, (struct sockaddr*)&myskt, sizeof(myskt)) < 0){
        perror("bind");
        exit(1);
    }
    
	bzero(&head, sizeof(head));
    
	client_list.fp = &client_list;
	client_list.bp = &client_list;

    for(;;){
        if((count = recvfrom(s, &head, sizeof(struct dhcph), 0, (struct sockaddr*)&skt, &sktlen)) < 0){
            perror("recvfrom");
            exit(1);
        }

		// get client
		cli = get_client(&skt.sin_addr);
        
        switch(cli->stat) {
            case STAT_WAIT_DISCOVER:
                if(head.type == DHCPDISCOVER) {
					struct in_addr ip;
					uint32_t mask;
					fprintf(stderr, "from STAT_WAIT_DISCOVER to STAT_WAIT_REQUEST\n");
                    // IPアドレスの選択、DHCPOFFERの返信
					queue_pop(&ip, &mask);
                    cli->stat = STAT_WAIT_REQUEST;
					cli->alloc_addr = ip;
					cli->netmask = mask;
					bzero(&head, sizeof(head));
					head.type = DHCPOFFER;
					head.address = ip.s_addr;
					head.netmask = mask;
					if ((count = sendto(s, &head, sizeof(struct dhcph), 0, (struct sockaddr*)&skt, sktlen)) < 0){
						perror("sendto");
						exit(1);
					}
                } else {
                    
                }
                break;
            case STAT_WAIT_REQUEST:
                if(head.type == DHCPREQUEST){
					fprintf(stderr, "from STAT_WAIT_REQUEST to STAT_WAIT_RELEASE\n");
					print_dhcp_header(&head);
					print_client(cli);
                    // DHCPACKの返信
                    cli->stat = STAT_WAIT_RELEASE;
					bzero(&head, sizeof(head));
					head.type = DHCPACK;
					if ((count = sendto(s, &head, sizeof(struct dhcph), 0, (struct sockaddr*)&skt, sktlen)) < 0){
						perror("sendto");
						exit(1);
					}
                } else {
                    
                }
                break;
			case STAT_WAIT_RELEASE:
				if(head.type == DHCPREQUEST){
					fprintf(stderr, "keeping STAT_WAIT_RELEASE\n");
					bzero(&head, sizeof(head));
					head.type = DHCPACK;
					if ((count = sendto(s, &head, sizeof(struct dhcph), 0, (struct sockaddr*)&skt, sktlen)) < 0){
						perror("sendto");
						exit(1);
					}
				} else if(head.type == DHCPRELEASE){
					fprintf(stderr, "release client\n");
					cli->stat = STAT_WAIT_DISCOVER;
				} else {
				}
				break;			
        }
    }
    
    if(close(s) < 0){
        perror("close");
        exit(1);
    }

	free(cli);
}

struct client* get_client(struct in_addr* ip)
{
	struct client* c = &client_list;

	while((c = c->fp) != &client_list){
		if(c->cli_id.s_addr == ip->s_addr){
			break;
		}
	}

	if(c == &client_list){
		struct client *n = create_client();
		n->cli_id = *ip;
		c = n;
	}

	return c;
}

struct client* create_client()
{
	struct client *n = (struct client*)malloc(sizeof(struct client));
	n->stat = STAT_WAIT_DISCOVER;
	client_list.bp->fp = n;
	n->bp = client_list.bp;
	n->fp = &client_list;
	client_list.bp = n;
	return n;
}

void print_client(struct client *c)
{
	fprintf(stderr, "Client IP Address: %s\n", inet_ntoa(c->cli_id));
	fprintf(stderr, "Given IP Address: %s\n", inet_ntoa(c->alloc_addr));
	fprintf(stderr, "Given Netmask: %d\n", c->netmask);
}

void read_config(char* file)
{
	char line[512];
	char ip[56];
	char mask[56];
	struct in_addr ipaddr;
	uint32_t netmask;
	FILE *fp;

	queue_init();

	if((fp = fopen(file, "r")) == NULL){
		fprintf(stderr, "cannot open config file\n");
		exit(1);
	}

	bzero(line, 512);

	while(fgets(line, 512 - 1, fp) != NULL){
		bzero(ip, 56);
		bzero(mask, 56);
		sscanf(line, "%55s %55s", ip, mask);
		inet_aton(ip, &ipaddr);
		netmask = (uint32_t)atoi(mask);
		queue_push(ipaddr, netmask);
		bzero(line, 512);
	}

	fclose(fp);

	debug_print();
}
