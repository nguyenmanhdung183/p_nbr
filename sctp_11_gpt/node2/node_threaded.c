#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define BUFFER_SIZE 1024

int sock;
struct sockaddr_in peer_addr;
socklen_t peer_len;

void *recv_thread_func(void *arg) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len;
    int n;

    while (1) {
        from_len = sizeof(from_addr);
        n = sctp_recvmsg(sock, buffer, BUFFER_SIZE, (struct sockaddr *)&from_addr, &from_len, NULL, NULL);
        if (n < 0) {
            perror("sctp_recvmsg");
            continue;
        }

        buffer[n] = '\0';
        printf("\nReceived from %s:%d: %s\n> ",
               inet_ntoa(from_addr.sin_addr), ntohs(from_addr.sin_port), buffer);
        fflush(stdout);
    }

    return NULL;
}

void *send_thread_func(void *arg) {
    char buffer[BUFFER_SIZE];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            perror("fgets");
            continue;
        }

        // Xóa ký tự newline cuối nếu có
        size_t len = strlen(buffer);
        if (buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        if (strlen(buffer) == 0)
            continue;

        if (sctp_sendmsg(sock, buffer, strlen(buffer),
                         (struct sockaddr *)&peer_addr, peer_len,
                         0, 0, 0, 0, 0) < 0) {
            perror("sctp_sendmsg");
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <local_port> <peer_ip> <peer_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int local_port = atoi(argv[1]);
    const char *peer_ip = argv[2];
    int peer_port = atoi(argv[3]);

    sock = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        close(sock);
        exit(EXIT_FAILURE);
    }

    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    peer_addr.sin_addr.s_addr = inet_addr(peer_ip);
    peer_len = sizeof(peer_addr);

    pthread_t recv_thread, send_thread;

    if (pthread_create(&recv_thread, NULL, recv_thread_func, NULL) != 0) {
        perror("pthread_create recv_thread");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&send_thread, NULL, send_thread_func, NULL) != 0) {
        perror("pthread_create send_thread");
        close(sock);
        exit(EXIT_FAILURE);
    }

    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);

    close(sock);
    return 0;
}
