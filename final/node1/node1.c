// node1.c

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>

#include <pthread.h>



#define MY_PORT     5000
#define PEER_PORT   5001

int sockfd;

void *receive_thread(void *arg) {
    struct sockaddr_in from_addr;
    socklen_t from_len;
    struct sctp_sndrcvinfo sinfo;
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        from_len = sizeof(from_addr);
        int n = sctp_recvmsg(sockfd, buffer, sizeof(buffer),
                             (struct sockaddr *)&from_addr, &from_len,
                             &sinfo, NULL);
        if (n > 0) {
            buffer[n] = '\0';
            printf("\n[Received from %s:%d]: %s\n",
                   inet_ntoa(from_addr.sin_addr),
                   ntohs(from_addr.sin_port),
                   buffer);
            printf("Enter message: "); fflush(stdout);
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in my_addr, peer_addr;
    pthread_t recv_tid;
    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MY_PORT);
    inet_pton(AF_INET, "127.0.0.1", &my_addr.sin_addr);

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Start receiving thread
    pthread_create(&recv_tid, NULL, receive_thread, NULL);

    // Prepare peer address (localhost:5001)
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(PEER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &peer_addr.sin_addr);

    // Input loop
    while (1) {
        printf("Enter message: ");
        fgets(buffer, sizeof(buffer), stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // remove newline

        if (strlen(buffer) == 0) continue;

        sctp_sendmsg(sockfd, buffer, strlen(buffer),
                     (struct sockaddr *)&peer_addr, sizeof(peer_addr),
                     0, 0, 0, 0, 0);
    }

    close(sockfd);
    return 0;
}
