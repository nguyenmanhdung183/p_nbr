#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#define SERVER_PORT 36422
#define BUFFER_SIZE 2048

int main() {
    int listen_fd, conn_fd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char buffer[BUFFER_SIZE];
    struct sctp_sndrcvinfo sinfo;

    listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (listen_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if (bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf(" Server is listening on port %d...\n", SERVER_PORT);

    len = sizeof(cliaddr);
    conn_fd = accept(listen_fd, (struct sockaddr *)&cliaddr, &len);
    if (conn_fd < 0) {
        perror("accept");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    int rcv_len = sctp_recvmsg(conn_fd, buffer, sizeof(buffer), NULL, 0, &sinfo, 0);
    if (rcv_len > 0) {
        printf(" Received %d bytes, PPID: %u\n", rcv_len, ntohl(sinfo.sinfo_ppid));
        printf(" Data (hex):\n");
        for (int i = 0; i < rcv_len; ++i) {
            printf("%02x ", (unsigned char)buffer[i]);
            if ((i+1) % 16 == 0) printf("\n");
        }
        printf("\n");
    } else {
        perror("sctp_recvmsg");
    }


    while(1){}
    close(conn_fd);
    close(listen_fd);
    return 0;
}
