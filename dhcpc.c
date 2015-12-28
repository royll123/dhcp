#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dhcp_common.h"

#define STAT_INITIAL	0
#define STAT_WAIT_OFFER 1
#define STAT_WAIT_REPLY 2
#define STAT_WAIT_TIME	3

int main(int argc, char* argv[])
{
	int s, count;
	struct sockaddr_in skt;
	socklen_t sktlen;
	in_port_t port = DHCP_PORT;
	struct in_addr ipaddr;
	int status;
	struct dhcph head;

	if(argc != 2){
		fprintf(stderr, "Usage: mydhcpc <server_ip_address>\n");
		exit(1);
	}

	inet_aton(argv[1], &ipaddr);

	if((s = socket(PF_INET, SOCK_DGRAM, 0)) == -1){
        perror("socket");
        exit(1);
    }

	skt.sin_family = AF_INET;
	skt.sin_port = htons(port);
	skt.sin_addr.s_addr = htonl(ipaddr.s_addr);
	status = STAT_INITIAL;

	for(;;){
		switch(status){
			case STAT_INITIAL:
				fprintf(stderr, "STAT_INITIAL\n");
				bzero(&head, sizeof(struct dhcph));
				head.type = DHCPDISCOVER;

				if ((count = sendto(s, &head, sizeof(head), 0, (struct sockaddr *)&skt, sizeof skt)) < 0) {
        			perror("sendto");
   			    	exit(1);
   				}
				status = STAT_WAIT_OFFER;
				break;
			case STAT_WAIT_OFFER:
				fprintf(stderr, "STAT_WAIT_OFFER\n");
				if ((count = recvfrom(s, &head, sizeof(struct dhcph), 0, (struct sockaddr *)&skt, &sktlen)) < 0) {
					perror("recvfrom");
					exit(1);
				}
				if(head.type == DHCPOFFER){
					bzero(&head, sizeof(struct dhcph));
					head.type = DHCPREQUEST;
					if ((count = sendto(s, &head, sizeof(head), 0, (struct sockaddr *)&skt, sizeof skt)) < 0) {
						perror("sendto");
						exit(1);
					}
					status = STAT_WAIT_REPLY;
				}
				break;
			case STAT_WAIT_REPLY:
				fprintf(stderr, "STAT_WAIT_REPLY\n");
				if ((count = recvfrom(s, &head, sizeof(struct dhcph), 0, (struct sockaddr *)&skt, &sktlen)) < 0) {
					perror("recvfrom");
					exit(1);
				}
				if(head.type == DHCPACK){
					status = STAT_WAIT_TIME;
				} else {
				}
				break;
			case STAT_WAIT_TIME:
				fprintf(stderr, "STAT_WAIT_TIME\n");
				break;
		}
	}

	if(close(s) < 0){
		perror("close");
		exit(1);
	}
}
