//server Local
//client Peer ---> this is PEER :)))) 256 thread send REQ to 1 Local -> send to p 36422

/*

256 luồng để gửi REQ cho 1 Local -> gửi đến port listening của PEER



*/

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


#define LOG_DETAIL 1
#undef LOG_DETAIL
#define MAX_BUFF 2048


volatile int resp_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

#define SERVER_IP "127.0.0.1"
#define NUM_PEER 250

const char *PEER_IP[NUM_PEER];
void init_peer_ips()
{
    for (int i = 0; i < NUM_PEER; i++)
    {
        char *ip = malloc(16);
        snprintf(ip, 16, "127.0.0.%d", i + 2); // từ 127.0.0.2 đến 127.0.0.251
        PEER_IP[i] = ip;
    }
}



#define SERVER_PORT 36422
#define NUM_CLIENT 250
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

void *client_thread(void *arg) {
    int idx = *(int *)arg;
    free(arg);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP); 
    if (sock < 0) {
        perror("socket");
        return NULL;
    }

    // Tùy chọn: bind source IP nếu  có nhiều IP trên máy
    
    struct sockaddr_in localaddr = {0};
    localaddr.sin_family = AF_INET;
    //localaddr.sin_port = htons(0); // port ngẫu nhiên
    localaddr.sin_port = htons(SERVER_PORT);
    char ip_str[32];
    snprintf(ip_str, sizeof(ip_str), "127.0.0.%d", (idx % 250) + 2);
    if (inet_pton(AF_INET, ip_str, &localaddr.sin_addr) > 0) {
        int reuse = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        if (bind(sock, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0) {
            perror("bind thất bại");
        } else {
            printf("bind thành công với port %d\n", SERVER_PORT);
        }
    }
    

    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    // if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
    //     perror("inet_pton server");
    //     close(sock);
    //     return NULL;
    // }
    servaddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET,  SERVER_IP, &servaddr.sin_addr) <= 0) {
        perror("inet_pton server");
        close(sock);
        return NULL;
    }
    if (connect(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sock);
        return NULL;
    }

    unsigned char req_bin[BUFFER_SIZE];
    int req_len = hexstr_to_bin(SETUP_REQ_HEX_STREAM, req_bin, sizeof(req_bin));
    if (req_len < 0) {
        fprintf(stderr, "Invalid SETUP_REQ_HEX_STREAM\n");
        close(sock);
        return NULL;
    }

    // ssize_t sent = send(sock, req_bin, req_len, 0);
    // if (sent < 0) {
    //     perror("send");
    //     close(sock);
    //     return NULL;
    // }
    // printf("[Client %d] Sent REQ %zd bytes\n", idx, sent);

    // Thay thế đoạn gửi dữ liệu
    ssize_t sent = sctp_sendmsg(sock,
                                req_bin,
                                req_len,
                                NULL, 0,            // addr, addrlen (NULL vì đã connect)
                                0,                  // ppid
                                0,                  // flags
                                0,                  // stream_no
                                0,                  // timetolive
                                0);                 // context

    if (sent < 0) {
        perror("sctp_sendmsg");
        close(sock);
        return NULL;
    }
    printf("[Client %d] Sent REQ %zd bytes\n", idx, sent);


    unsigned char resp[BUFFER_SIZE];

    
    // ssize_t n = recv(sock, resp, sizeof(resp), 0);
//-------------
    struct sockaddr_in from;
socklen_t from_len = sizeof(from);
struct sctp_sndrcvinfo info;  // phải khai báo biến info
int stream_no, flags, ppid;
ssize_t n = sctp_recvmsg(sock,
                         resp,
                         sizeof(resp),
                         (struct sockaddr *)&from,
                         &from_len,
                         &info,
                         &flags);

if (n < 0) {
    perror("sctp_recvmsg");
    close(sock);
    return NULL;
}
//-----------

    unsigned char resp_bin[BUFFER_SIZE];
    int resp_len = hexstr_to_bin(SETUP_RESP_HEX_STREAM, resp_bin, sizeof(resp_bin));
    if (resp_len < 0) {
        fprintf(stderr, "Invalid SETUP_RESP_HEX_STREAM\n");
        close(sock);
        return NULL;
    }

    bool is_setup_resp = (n == resp_len) && (memcmp(resp, resp_bin, (size_t)n) == 0);
        if (is_setup_resp) {
            #ifdef LOG_DETAIL

            printf("[Client %d] Matched expected RESP pattern.\n", idx);
            #endif
        } else {
            printf("[Client %d] RESP pattern did not match expected.\n", idx);
        }

    if (n > 0) {
        printf("[Client %d] Received RESP %zd bytes:\n\n", idx, n);
            pthread_mutex_lock(&count_mutex);
            resp_count++;
            pthread_mutex_unlock(&count_mutex);
#ifdef LOG_DETAIL

        print_hex(resp, (int)n);
#endif

    } else if (n == 0) {
        printf("[Client %d] Server closed connection\n", idx);
    } else {
        perror("[Client recv]");
    }

    close(sock);
    return NULL;
}

int main() {
    pthread_t threads[NUM_CLIENT];
init_peer_ips();
    for (int i = 0; i < NUM_CLIENT; ++i) {
        int *p = malloc(sizeof(int));
        if (!p) { perror("malloc"); return 1; }
        *p = i;
        if (pthread_create(&threads[i], NULL, client_thread, p) != 0) {
            perror("pthread_create");
            free(p);
        }
        usleep(5000);
    }

    for (int i = 0; i < NUM_CLIENT; ++i) {
        pthread_join(threads[i], NULL);
    }
    printf("Total valid RESP received: %d\n", resp_count);
    return 0;
}
