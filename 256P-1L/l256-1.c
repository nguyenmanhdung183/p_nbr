//server Local   ----> this is LOCAL :))))
//client Peer

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_BUFF 2048

#define LOG_DETAIL 1
#undef LOG_DETAIL

#define SERVER_PORT 36422
#define LISTEN_BACKLOG 256
#define BUFFER_SIZE 2048

#define SETUP_REQ_HEX_STREAM "0024004100000100f4003a000002001500080054f24000396a4000fa0027000800020054f240396a4006560054f2400051d60b863300010029400100003740050000000000"
#define SETUP_RESP_HEX_STREAM "2024004200000100f6003b000002001500080054f24000396a4000fa0028000800020054f240396a4003400054f240004cf406a43300010029400120003740064002659020c0" // ví dụ

int hexstr_to_bin(const char *hexstr, unsigned char *buf, size_t max_len) {
    size_t len = strlen(hexstr);
    if (len % 2 != 0 || len / 2 > max_len) return -1;
    for (size_t i = 0; i < len/2; ++i) {
        unsigned int byte;
        if (sscanf(hexstr + 2*i, "%2x", &byte) != 1) return -1;
        buf[i] = (unsigned char)byte;
    }
    return (int)(len/2);
}

void print_hex(const unsigned char *buf, int len) {
    for (int i = 0; i < len; ++i) {
        printf("%02x", buf[i]);
        if ((i+1) % 16 == 0) printf("\n");
        else if ((i+1) % 2 == 0) printf(" ");
    }
    if (len % 16) printf("\n");
}

void *client_handler(void *arg) {
    int connfd = *(int *)arg;
    free(arg);

    unsigned char buf[BUFFER_SIZE];
    // ssize_t n = recv(connfd, buf, sizeof(buf), 0);
    // if (n <= 0) {
    //     if (n < 0) perror("recv");
    //     close(connfd);
    //     return NULL;
    // }
///------------

struct sockaddr_in from;
socklen_t from_len = sizeof(from);
struct sctp_sndrcvinfo info;
int flags;

ssize_t n = sctp_recvmsg(connfd,
                         buf,
                         sizeof(buf),
                         (struct sockaddr *)&from,
                         &from_len,
                         &info,
                         &flags);

if (n <= 0) {
    if (n < 0) perror("sctp_recvmsg");
    close(connfd);
    return NULL;
}
///------------
    
#ifdef LOG_DETAIL
    printf("=== New connection, received %zd bytes ===\n", n);
    print_hex(buf, (int)n);
#endif

    unsigned char req_bin[BUFFER_SIZE];
    int req_len = hexstr_to_bin(SETUP_REQ_HEX_STREAM, req_bin, sizeof(req_bin));
    if (req_len < 0) {
        fprintf(stderr, "Invalid SETUP_REQ_HEX_STREAM\n");
        close(connfd);
        return NULL;
    }

    bool is_setup_req = (n == req_len) && (memcmp(buf, req_bin, (size_t)n) == 0);
    if (is_setup_req) {
        #ifdef LOG_DETAIL
        printf("Matched SETUP REQUEST pattern. Sending SETUP RESPONSE...\n");
        #else
        printf("Received SETUP REQUEST ");
        printf("Sending SETUP RESPONSE...\n");
        #endif

    } else {
        printf("Not matched SETUP REQUEST pattern. Still sending response (optional behavior).\n");
    }

    unsigned char resp_bin[BUFFER_SIZE];
    int resp_len = hexstr_to_bin(SETUP_RESP_HEX_STREAM, resp_bin, sizeof(resp_bin));
    if (resp_len < 0) {
        fprintf(stderr, "Invalid SETUP_RESP_HEX_STREAM\n");
        close(connfd);
        return NULL;
    }

    // ssize_t sent = send(connfd, resp_bin, resp_len, 0);
    // if (sent < 0) perror("send");
    // else printf("Sent response (%zd bytes)\n\n\n", sent);

    ssize_t sent = sctp_sendmsg(connfd,
                                resp_bin,
                                resp_len,
                                NULL, 0,
                                0, 0, 0, 0, 0);

    if (sent < 0) perror("sctp_sendmsg");
    else printf("Sent response (%zd bytes)\n\n\n", sent);

    close(connfd);
    return NULL;
}

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP); 
    if (listenfd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(listenfd);
        return 1;
    }

    if (listen(listenfd, LISTEN_BACKLOG) < 0) {
        perror("listen");
        close(listenfd);
        return 1;
    }

    printf("Local server listening on port %d\n", SERVER_PORT);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int *connfd = malloc(sizeof(int));
        if (!connfd) { perror("malloc"); continue; }
        *connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (*connfd < 0) {
            perror("accept");
            free(connfd);
            continue;
        }

        char cli_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliaddr.sin_addr, cli_ip, sizeof(cli_ip));
        printf("Accepted from %s:%d\n", cli_ip, ntohs(cliaddr.sin_port));

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, connfd) != 0) {
            perror("pthread_create");
            close(*connfd);
            free(connfd);
            continue;
        }
        pthread_detach(tid);
    }

    close(listenfd);
    return 0;
}
