// server: Peer
// client Local  -----This is Local :))) gửi REQ đến 256 Peer
///-----> gửi REQ cho 256 Peer -> dùng 1 Port x2 36422 gửi đến port kia   --> tách 256 thread để nhận 256 RESP và tăng biến đếm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

#define LOG_DETAIL 1
#undef LOG_DETAIL

#define NUM_PEER 250
#define X2_PORT 36422 //// gửi đến port 36422 của 256 ip của peer
volatile int resp_count = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *PEER_IP[NUM_PEER];

int sockfd[NUM_PEER];

#define PEER_PORT_START 5000
#define PEER_PORT_END (PEER_PORT_START + NUM_PEER)

#define SETUP_REQ_HEX_STREAM "0024004100000100f4003a000002001500080054f24000396a4000fa0027000800020054f240396a4006560054f2400051d60b863300010029400100003740050000000000"
#define SETUP_RESP_HEX_STREAM "0200004100000100f4003a000002001500080054f240" // ví dụ giả lập

void init_peer_ips()
{
    for (int i = 0; i < NUM_PEER; i++)
    {
        char *ip = malloc(16);
        snprintf(ip, 16, "127.0.0.%d", i + 2); // từ 127.0.0.2 đến 127.0.0.251
        PEER_IP[i] = ip;
    }
}

void free_peer_ips()
{
    for (int i = 0; i < NUM_PEER; i++)
    {
        free((void *)PEER_IP[i]);
    }
}

int hexstr_to_bin(const char *hexstr, unsigned char *buf, size_t max_len)
{
    size_t len = strlen(hexstr);
    if (len % 2 != 0 || len / 2 > max_len)
        return -1;
    for (size_t i = 0; i < len / 2; ++i)
        sscanf(hexstr + 2 * i, "%2hhx", &buf[i]);
    return len / 2;
}

void print_hex(const unsigned char *buf, int len)
{
    for (int i = 0; i < len; ++i)
    {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

// Hàm thread nhận response từ peer
void *recv_resp_thread(void *arg)
{
    int idx = *(int *)arg;
    free(arg);

    unsigned char buf[1024];
    while (1)
    {
        int n = recv(sockfd[idx], buf, sizeof(buf), 0);
        unsigned char temp_buf[1024];
        int temp_h2b = hexstr_to_bin(SETUP_RESP_HEX_STREAM, temp_buf, sizeof(temp_buf));
        if (temp_h2b < 0)
        {
            fprintf(stderr, "Invalid hex string\n");
            close(sockfd[idx]);
            return NULL;
        }

        bool is_setup_req = (n == temp_h2b) && (memcmp(buf, temp_buf, n) == 0);
#ifdef LOG_DETAIL

        if (!is_setup_req)
        {
            printf("Not X2 Setup RESP, close connection\n");
            close(sockfd[idx]);
            return NULL;
        }
        else
        {
            printf("Is X2 Setup RESP\n");
        }
#endif
        if (n > 0)
        {
#ifdef LOG_DETAIL
            printf("[Peer %d - %s] Received response (%d bytes):\n", idx + 1, PEER_IP[idx], n);
            print_hex(buf, n);
#else
            printf("[Peer %d - %s] Received response (%d bytes):\n", idx + 1, PEER_IP[idx], n);

#endif
            pthread_mutex_lock(&count_mutex);
            resp_count++;
            pthread_mutex_unlock(&count_mutex);
        }
        else if (n == 0)
        {
#ifdef LOG_DETAIL
            printf("[Peer %d - %s] Connection closed by peer\n", idx + 1, PEER_IP[idx]);
#endif
            close(sockfd[idx]);
            sockfd[idx] = -1;
            break;
        }
        else
        {
            if (errno == EINTR)
                continue;
            perror("recv");
            break;
        }
    }
    return NULL;
}

// Gửi X2 Setup Request đến peer thứ idx
int send_x2_req(int idx)
{
    struct sockaddr_in servaddr;

    sockfd[idx] = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd[idx] < 0)
    {
        perror("socket");
        return -1;
    }

    //--port local
    // struct sockaddr_in localaddr;
    // memset(&localaddr, 0, sizeof(localaddr));
    // localaddr.sin_family = AF_INET;
    // localaddr.sin_addr.s_addr = htonl(INADDR_ANY); // hoặc inet_addr("127.0.0.1")
    // localaddr.sin_port = htons(X2_PORT);           // 36422

    // if (bind(sockfd[idx], (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0)
    // {
    //     perror("bind");
    //     close(sockfd[idx]);
    //     sockfd[idx] = -1;
    //     return -1;
    // }

    // e --port local

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(X2_PORT);

    if (inet_pton(AF_INET, PEER_IP[idx], &servaddr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sockfd[idx]);
        sockfd[idx] = -1;
        return -1;
    }

    if (connect(sockfd[idx], (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) // connect đến port 36422 của peer
    {                                                                             // rd port
        perror("connect");
        close(sockfd[idx]);
        sockfd[idx] = -1;
        return -1;
    }

    unsigned char buffer[1024];
    int payload_len = hexstr_to_bin(SETUP_REQ_HEX_STREAM, buffer, sizeof(buffer));
    if (payload_len < 0)
    {
        fprintf(stderr, "Invalid hex string\n");
        close(sockfd[idx]);
        sockfd[idx] = -1;
        return -1;
    }

    // PPID = 46 (X2AP)
    int ret = sctp_sendmsg(sockfd[idx], buffer, payload_len,
                           NULL, 0,
                           htonl(46), 0, 0, 0, 0);
    if (ret < 0)
    {
        perror("sctp_sendmsg");
        close(sockfd[idx]);
        sockfd[idx] = -1;
        return -1;
    }

    printf("[Peer %d - %s] Sent X2 Setup Request (%d bytes)\n", idx + 1, PEER_IP[idx], ret);
    return 0;
}

int main()
{
    printf("local ENB -> send 256 REQ to 256 PEER\n");
    init_peer_ips();

    // Tạo socket, connect và gửi X2 Setup Req cho từng peer
    for (int i = 0; i < NUM_PEER; i++)
    {
        if (send_x2_req(i) != 0)
        {
            fprintf(stderr, "[Peer %d] Failed to send X2 Setup Request\n", i + 1);
        }
    }

    pthread_t tid[NUM_PEER];
    for (int i = 0; i < NUM_PEER; i++)
    {
        if (sockfd[i] < 0)
            continue;
        int *idx = malloc(sizeof(int));
        *idx = i;
        if (pthread_create(&tid[i], NULL, recv_resp_thread, idx) != 0)
        {
            perror("pthread_create");
            free(idx);
            close(sockfd[i]);
            sockfd[i] = -1;
        }
    }

    for (int i = 0; i < NUM_PEER; i++)
    {
        if (sockfd[i] >= 0)
        {
            pthread_join(tid[i], NULL);
        }
    }
    printf("Total responses received: %d\n", resp_count);

    free_peer_ips();
    return 0;
}