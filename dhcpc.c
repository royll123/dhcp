#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dhcp_common.h"

#define STAT_WAIT_OFFER 1
#define STAT_WAIT_REPLY 2


int main(int argc, char* argv[])
{
	int s, count, datalen;
	struct sockaddr_in skt;
	char sbuf[512] = {0};
	in_port_t port = DHCP_PORT;
	struct in_addr ipaddr;

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

	datalen = 3;
	if ((count = sendto(s, sbuf, datalen, 0, (struct sockaddr *)&skt, sizeof skt)) < 0) {
		perror("sendto");
		exit(1);
	}

	if(close(s) < 0){
		perror("close");
		exit(1);
	}
}
