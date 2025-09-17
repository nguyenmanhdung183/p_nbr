// x2_sctp_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>

int main() {
    int sock = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5000);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);

    char *msg = "Hello from client (1-N)";
    sctp_sendmsg(sock, msg, strlen(msg), (struct sockaddr*)&servaddr,
                 sizeof(servaddr), 0, 0, 0, 0, 0);

    char buffer[100];
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    int n = sctp_recvmsg(sock, buffer, sizeof(buffer)-1,
                         (struct sockaddr*)&from, &from_len, NULL, NULL);
    buffer[n] = '\0';
    printf("Received: %s\n", buffer);

    close(sock);
    return 0;
}