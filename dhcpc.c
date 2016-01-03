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
void print_allocated_address(in_addr_t ip, uint32_t netmask, uint16_t ttl);

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
	fprintf(stderr, "STAT_INITIAL\n");

	for(;;){
		if(kill_flag == 1){
			bzero(&head, sizeof(struct dhcph));
			head.type = DHCPRELEASE;
			if((count = sendto(s, &head, sizeof(head), 0, (struct sockaddr *)&skt, sizeof skt)) < 0){
				perror("sendto");
				exit(1);
			}
			fprintf(stderr, "Send\n");
			print_dhcp_header(&head);
			break;
		}

		switch(status){
			case STAT_INITIAL:
				bzero(&head, sizeof(struct dhcph));
				head.type = DHCPDISCOVER;

				if ((count = sendto(s, &head, sizeof(head), 0, (struct sockaddr *)&skt, sizeof skt)) < 0) {
        			perror("sendto");
   			    	exit(1);
   				}
				fprintf(stderr, "Send\n");
				print_dhcp_header(&head);
				status = STAT_WAIT_OFFER;
				fprintf(stderr, "from STAT_INITIAL to STAT_WAIT_OFFER\n");
				break;
			case STAT_WAIT_OFFER:
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
					fprintf(stderr, "from STAT_WAIT_OFFER to STAT_INITIAL\n");
					break;
				}
				if(FD_ISSET(s, &rdfds)){
					if ((count = recvfrom(s, &head, sizeof(struct dhcph), 0, (struct sockaddr *)&skt, &sktlen)) < 0) {
						perror("recvfrom");
						exit(1);
					}

					fprintf(stderr, "Receive\n");
					print_dhcp_header(&head);

					if(head.type == DHCPOFFER){
						if(head.code == DHCP_CODE_OK){
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
							fprintf(stderr, "Send\n");
							print_dhcp_header(&head);
							status = STAT_WAIT_REPLY;
							fprintf(stderr, "from STAT_WAIT_OFFER to STAT_WAIT_REPLY\n");
						} else if(head.code == DHCP_CODE_ERR_NONE){
							fprintf(stderr, "ERROR:%d There is no ip resource. Exit.\n", head.code);
							exit(1);
						} else {
							fprintf(stderr, "Received invalid DHCP code. Exit.\n");
							exit(1);
						}
					} else {
						fprintf(stderr, "Recieved invalid data. Ignore.\n");
					}
				}
				break;
			case STAT_WAIT_REPLY:
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
					fprintf(stderr, "from STAT_WAIT_REPLY to STAT_INITIAL\n");
                    break;
                }
                if(FD_ISSET(s, &rdfds)){
					if ((count = recvfrom(s, &head, sizeof(struct dhcph), 0, (struct sockaddr *)&skt, &sktlen)) < 0) {
						perror("recvfrom");
						exit(1);
					}

					fprintf(stderr, "Receive\n");
					print_dhcp_header(&head);

					if(head.type == DHCPACK){
						if(head.code == DHCP_CODE_OK){
							status = STAT_WAIT_TIME;
							fprintf(stderr, "from STAT_WAIT_REPLY to STAT_WAIT_TIME\n");
							my_ttl = head.ttl;
							my_addr = head.address;
							my_netmask = head.netmask;
							alarm(my_ttl/2);
							print_allocated_address(my_addr, my_netmask, my_ttl);
						} else if(head.code == DHCP_CODE_ERR_OVL){
							fprintf(stderr, "ERROR:%d Already Allocated IP Address.\n", head.code);
							status = STAT_INITIAL;
							fprintf(stderr, "from STAT_WAIT_REPLY to STAT_INITIAL\n");
						} else {
							fprintf(stderr, "Received invalid DHCP data. Ignore.\n");
						}
					} else {
						fprintf(stderr, "Received invalid data. Ignore.\n");
					}
				}
				break;
			case STAT_WAIT_TIME:
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
					fprintf(stderr, "Send\n");
					print_dhcp_header(&head);
					status = STAT_WAIT_REPLY;
					fprintf(stderr, "from STAT_WAIT_TIME to STAT_WAIT_REPLY\n");
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
	alrm_flag = 1;
}

void kill_process(int sig)
{
	kill_flag = 1;
}

void print_allocated_address(in_addr_t ip, uint32_t netmask, uint16_t ttl)
{
	struct in_addr i = {ip};
	fprintf(stderr, "---  Allocated IP Address  ---\n");
	fprintf(stderr, "IP: %s\n", inet_ntoa(i));
	fprintf(stderr, "Netmask: %d\n", netmask);
	fprintf(stderr, "Time to Live: %d\n", ttl);
	fprintf(stderr, "---           end          ---\n");
}
