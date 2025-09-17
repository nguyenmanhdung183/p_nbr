// x2_sctp_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

int main() {
    int sock = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5000);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr));

    printf("One-to-Many server listening...\n");

    char buffer[100];
    int n=0;
    while(1){
            n = sctp_recvmsg(sock, buffer, sizeof(buffer)-1,
                         (struct sockaddr*)&cliaddr, &len, NULL, NULL);
                         if(n>0) break;
    }

    buffer[n] = '\0';
    printf("Received: %s\n", buffer);

    char *reply = "Reply from server (1-N)";
    sctp_sendmsg(sock, reply, strlen(reply),
                 (struct sockaddr*)&cliaddr, len, 0, 0, 0, 0, 0);

    close(sock);
    return 0;
}