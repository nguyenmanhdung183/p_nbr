// server: Peer -----This is Peer :))) nhận REQ từ Local
// client Local

///-> tách 256 thread để nhận REQ từ Client và gửi lại RESP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

#define LOG_DETAIL 1
#undef LOG_DETAIL

#define NUM_PEER 250
#define X2_PORT 36422 

#define SETUP_REQ_HEX_STREAM "0024004100000100f4003a000002001500080054f24000396a4000fa0027000800020054f240396a4006560054f2400051d60b863300010029400100003740050000000000"
#define SETUP_RESP_HEX_STREAM "0200004100000100f4003a000002001500080054f240" // ví dụ giả lập

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

// Thread xử lý 1 kết nối peer
void *peer_thread(void *arg)
{
    int connfd = *(int *)arg;
    free(arg);

    unsigned char buffer[1024];
    ssize_t n;

    // Nhận X2 Setup Request
    n = recv(connfd, buffer, sizeof(buffer), 0);

    unsigned char temp_buf[1024];
    int temp_h2b = hexstr_to_bin(SETUP_REQ_HEX_STREAM, temp_buf, sizeof(temp_buf));
    if (temp_h2b < 0)
    {
        fprintf(stderr, "Invalid hex string\n");
        close(connfd);
        return NULL;
    }

    bool is_setup_req = (n == temp_h2b) && (memcmp(buffer, temp_buf, n) == 0);
#ifdef LOG_DETAIL

    if (!is_setup_req)
    {
        printf("Not X2 Setup Request, close connection\n");
        close(connfd);
        return NULL;
    }
    else
    {
        printf("Is X2 Setup Request\n");
    }
#endif

    if (n <= 0)
    {
        if (n == 0)
        {
#ifdef LOG_DETAIL
            printf("Peer closed connection\n");
#endif
        }
        else
        {
            perror("recv");
        }
        close(connfd);
        return NULL;
    }
#ifdef LOG_DETAIL
    printf("Received X2 Setup Request (%zd bytes):\n", n);
    print_hex(buffer, n);
#else
    printf("Received X2 Setup Request (%zd bytes):\n", n);

#endif

    // Gửi lại X2 Setup Response
    unsigned char resp_buf[1024];
    int resp_len = hexstr_to_bin(SETUP_RESP_HEX_STREAM, resp_buf, sizeof(resp_buf));
    if (resp_len < 0)
    {
        fprintf(stderr, "Invalid response hex string\n");
        close(connfd);
        return NULL;
    }

    ssize_t sent = send(connfd, resp_buf, resp_len, 0);
    if (sent < 0)
    {
        perror("send");
    }
    else
    {

#ifdef LOG_DETAIL
        printf("Sent X2 Setup Response (%zd bytes)\n\n\n", sent);
#else
        printf("Sent X2 Setup Response (%zd bytes)\n\n\n", sent);

#endif
    }

    close(connfd);
    return NULL;
}

int main()
{
    int listenfd;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (listenfd < 0)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(X2_PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind");
        close(listenfd);
        return 1;
    }

    // Listen với backlog 256 (số client chờ kết nối)
    if (listen(listenfd, NUM_PEER) < 0)
    {
        perror("listen");
        close(listenfd);
        return 1;
    }

    printf("Peer SCTP server listening on port %d\n", X2_PORT);

    while (1)
    {
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        int *connfd = malloc(sizeof(int));
        if (!connfd)
        {
            perror("malloc");
            continue;
        }

        *connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);
        if (*connfd < 0)
        {
            perror("accept");
            free(connfd);
            continue;
        }

        char cli_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cliaddr.sin_addr, cli_ip, sizeof(cli_ip));
        printf("New connection from %s:%d\n", cli_ip, ntohs(cliaddr.sin_port));

        pthread_t tid;
        if (pthread_create(&tid, NULL, peer_thread, connfd) != 0)
        {
            perror("pthread_create");
            close(*connfd);
            free(connfd);
            continue;
        }

        pthread_detach(tid); // tự động thu gom thread khi kết thúc
    }

    close(listenfd);
    return 0;
}
