#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <pthread.h>

#define SERVER_PORT 36422
#define BUFFER_SIZE 2048

void print_hex(const char *buf, int len) {
    for (int i = 0; i < len; ++i) {
        printf("%02x ", (unsigned char)buf[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

// Hàm xử lý client trong luồng riêng
void *handle_client(void *arg) {
    int conn_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    struct sctp_sndrcvinfo sinfo;
    int flags = 0;

    while (1) {
        int rcv_len = sctp_recvmsg(conn_fd, buffer, sizeof(buffer),
                                   NULL, 0, &sinfo, &flags);

        if (rcv_len <= 0) {
            if (rcv_len == 0) {
                printf("Client disconnected.\n");
            } else {
                perror("sctp_recvmsg");
            }
            break;
        }

        printf("---Received %d bytes, PPID: %u\n", rcv_len, ntohl(sinfo.sinfo_ppid));
        print_hex(buffer, rcv_len);

        // Gửi phản hồi "resp"
        const char *resp = "resp";
        int ret = sctp_sendmsg(conn_fd, resp, strlen(resp),
                               NULL, 0,
                               0, 0,
                               0, 0, 0);

        if (ret > 0) {
            printf("--- Sent response (%d bytes): %s\n", ret, resp);
        } else {
            perror("sctp_sendmsg");
            break;
        }
    }

    close(conn_fd);
    return NULL;
}

int main() {
    int listen_fd;
    struct sockaddr_in servaddr;

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

    printf("🌐 Server is listening on port %d (SCTP)...\n", SERVER_PORT);

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);

        int *conn_fd = malloc(sizeof(int));
        if (!conn_fd) {
            perror("malloc");
            continue;
        }

        *conn_fd = accept(listen_fd, (struct sockaddr *)&cliaddr, &len);
        if (*conn_fd < 0) {
            perror("accept");
            free(conn_fd);
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliaddr.sin_addr, ip_str, sizeof(ip_str));
        printf("🔗 New connection from %s:%d\n", ip_str, ntohs(cliaddr.sin_port));

        // Tạo luồng xử lý client
        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, conn_fd) != 0) {
            perror("pthread_create");
            close(*conn_fd);
            free(conn_fd);
        } else {
            pthread_detach(tid); // Tự dọn dẹp khi kết thúc
        }
    }

    close(listen_fd);
    return 0;
}
