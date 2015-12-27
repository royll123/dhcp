#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "dhcp_common.h"

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


int main(int argc, char* argv[])
{
    int s, msgtype, count;
    char buf[512];
    in_port_t myport = DHCP_PORT;
    struct client *cli;
    struct sockaddr_in myskt;
    struct sockaddr_in skt;
    socklen_t sktlen;
    
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
    
    
    for(;;){
        if((count = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&skt, &sktlen)) < 0){
            perror("recvfrom");
            exit(1);
        }
        
        switch(cli->stat) {
            case STAT_WAIT_DISCOVER:
				fprintf(stderr, "RECEIVED: STAT_WAIT_DISCOVER\n");
                if(msgtype == DHCPDISCOVER) {
                    // IPアドレスの選択、DHCPOFFERの返信
                    cli->stat = STAT_WAIT_REQUEST;
                } else {
                    
                }
                break;
            case STAT_WAIT_REQUEST:
				fprintf(stderr, "RECEIVED: STAT_WAIT_REQUEST\n");
                if(msgtype == DHCPREQUEST){
                    // DHCPACKの返信
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
}
