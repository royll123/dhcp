#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include "dhcp_common.h"

#define STAT_INITIAL	0
#define STAT_WAIT_OFFER 1
#define STAT_WAIT_REPLY 2
#define STAT_WAIT_TIME	3

void set_signal();
void timeout_handler(int);
void kill_process(int);

int alrm_flag = 0;
int kill_flag = 0;

int main(int argc, char* argv[])
{
	int s, count;
	struct sockaddr_in skt;
	socklen_t sktlen = sizeof(skt);
	in_port_t port = DHCP_PORT;
	struct in_addr ipaddr;
	int status;
	struct dhcph head;
	in_addr_t my_addr;
	uint32_t my_netmask;
	uint16_t my_ttl;
	int result;
	fd_set rdfds;
	struct timeval t;

	if(argc != 2){
		fprintf(stderr, "Usage: mydhcpc <server_ip_address>\n");
		exit(1);
	}

	set_signal();

	if(inet_pton(AF_INET, argv[1], &ipaddr) != 1){
		fprintf(stderr, "wrong server_ip_address\n");
		exit(1);
	}

	if((s = socket(PF_INET, SOCK_DGRAM, 0)) == -1){
        perror("socket");
        exit(1);
    }

	skt.sin_family = AF_INET;
	skt.sin_port = htons(port);
	skt.sin_addr = ipaddr;
	status = STAT_INITIAL;

	for(;;){
		if(kill_flag == 1){
			bzero(&head, sizeof(struct dhcph));
			head.type = DHCPRELEASE;
			if((count = sendto(s, &head, sizeof(head), 0, (struct sockaddr *)&skt, sizeof skt)) < 0){
				perror("sendto");
				exit(1);
			}
			break;
		}

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
				t.tv_sec = 10;
				t.tv_usec = 0;
				FD_ZERO(&rdfds);
				FD_SET(s, &rdfds);
				if((result = select(s+1, &rdfds, NULL, NULL, &t)) < 0){
					perror("select");
					exit(1);
				}
				if(result == 0){
					fprintf(stderr, "timeout\n");
					status = STAT_INITIAL;
					break;
				}
				if(FD_ISSET(s, &rdfds)){
					if ((count = recvfrom(s, &head, sizeof(struct dhcph), 0, (struct sockaddr *)&skt, &sktlen)) < 0) {
						perror("recvfrom");
						exit(1);
					}
					if(head.type == DHCPOFFER){
						if(head.code == DHCP_CODE_OK){
							print_dhcp_header(&head);
							my_addr = head.address;
							my_netmask = head.netmask;
							my_ttl = head.ttl;
							bzero(&head, sizeof(struct dhcph));
							head.type = DHCPREQUEST;
							head.code = DHCP_CODE_REQ_ALC;
							head.address = my_addr;
							head.netmask = my_netmask;
							head.ttl = my_ttl;
							if ((count = sendto(s, &head, sizeof(head), 0, (struct sockaddr *)&skt, sizeof skt)) < 0) {
								perror("sendto");
								exit(1);
							}
							status = STAT_WAIT_REPLY;
						} else if(head.code == DHCP_CODE_ERR_NONE){
							fprintf(stderr, "ERROR:%d\nThere is no ip resource.\nExit.\n", head.code);
							exit(1);
						} else {
							fprintf(stderr, "Received invalid DHCP code.\nExit.\n");
							exit(1);
						}
					} else {
						fprintf(stderr, "Recieved invalid data.\nIgnore.\n");
					}
				}
				break;
			case STAT_WAIT_REPLY:
				fprintf(stderr, "STAT_WAIT_REPLY\n");
				t.tv_sec = 10;
                t.tv_usec = 0;
                FD_ZERO(&rdfds);
                FD_SET(s, &rdfds);
                if((result = select(s+1, &rdfds, NULL, NULL, &t)) < 0){
                    perror("select");
                    exit(1);
                }
                if(result == 0){
                    fprintf(stderr, "timeout\n");
                    status = STAT_INITIAL;
                    break;
                }
                if(FD_ISSET(s, &rdfds)){
					if ((count = recvfrom(s, &head, sizeof(struct dhcph), 0, (struct sockaddr *)&skt, &sktlen)) < 0) {
						perror("recvfrom");
						exit(1);
					}
					if(head.type == DHCPACK){
						if(head.code == DHCP_CODE_OK){
							status = STAT_WAIT_TIME;
							my_ttl = head.ttl;
							my_addr = head.address;
							my_netmask = head.netmask;
							alarm(my_ttl/2);
						} else if(head.code == DHCP_CODE_ERR_OVL){
							fprintf(stderr, "ERROR:%d\n Already Allocated IP Address.\n", head.code);
							status = STAT_INITIAL;
						} else {
							fprintf(stderr, "Received invalid DHCP data.\nignore.\n");
						}
					} else {
						fprintf(stderr, "Received invalid data.\nignore.\n");
					}
				}
				break;
			case STAT_WAIT_TIME:
				 fprintf(stderr, "STAT_WAIT_TIME\n");

				 pause();

				 if(alrm_flag == 1){
				 	// request again
					alrm_flag = 0;
					bzero(&head, sizeof(struct dhcph));
					head.type = DHCPREQUEST;
					head.code = DHCP_CODE_REQ_EXT;
					head.address = my_addr;
					head.netmask = my_netmask;
					head.ttl = my_ttl;

					if((count = sendto(s, &head, sizeof(head), 0, (struct sockaddr *)&skt, sizeof(skt))) < 0){
						perror("sendto");
						exit(1);
					}
					status = STAT_WAIT_REPLY;
				 }
				break;
		}
	}

	if(close(s) < 0){
		perror("close");
		exit(1);
	}
}

void set_signal()
{
    struct sigaction alrm, ctrl;
    alrm.sa_handler = &timeout_handler;
    alrm.sa_flags = 0;
	ctrl.sa_handler = &kill_process;
	ctrl.sa_flags = 0;

    if(sigaction(SIGALRM, &alrm, NULL) < 0){
        perror("sigaction");
        exit(1);
    }

	if(sigaction(SIGINT, &ctrl, NULL) < 0){
		perror("sigaction");
		exit(1);
	}
}

void timeout_handler(int sig)
{
	fprintf(stderr, "alarm\n");
	alrm_flag = 1;
}

void kill_process(int sig)
{
	fprintf(stderr, "exit process\n");
	kill_flag = 1;
}
