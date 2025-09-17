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

#define NUM_PROCESSES 256

#define SERVER_IP "127.0.0.1" // sua thanh ip cua enb xem
#define SERVER_PORT 36422
#define STREAM_NO 0
#define X2AP_PPID 0


// Convert hex string to binary
int hexstr_to_bin(const char *hexstr, unsigned char *buf, size_t max_len)
{
    size_t len = strlen(hexstr);
    if (len % 2 != 0 || len / 2 > max_len)
        return -1;
    for (size_t i = 0; i < len / 2; ++i)
        sscanf(hexstr + 2 * i, "%2hhx", &buf[i]);
    return len / 2;
}

void run(sem_t *sem, int *counter)
{

    int sockfd;
    struct sockaddr_in servaddr;
    unsigned char buffer[1024];

    const char *hex_payload =
        "0024004100000100f4003a000002001500080054f24000396a4000fa0027000800020054f240396a4006560054f2400051d60b863300010029400100003740050000000000";

    int payload_len = hexstr_to_bin(hex_payload, buffer, sizeof(buffer));
    if (payload_len < 0)
    {
        fprintf(stderr, "Invalid hex string\n");
        return;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd < 0)
    {
        perror("socket");
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
        close(sockfd);
        return;
    }

    int ret = sctp_sendmsg(sockfd, buffer, payload_len,
                           NULL, 0,
                           0,
                           0,
                           STREAM_NO,
                           0,
                           0);

    if (ret < 0)
    {
        perror("sctp_sendmsg");
        close(sockfd);
        return;
    }

    printf(" Sent %d bytes using sctp_sendmsg().\n", ret);

    char buf[1024];
    int rcv_len = sctp_recvmsg(sockfd, buf, sizeof(buf), NULL, 0, 0, 0);
    if (rcv_len > 0)
    {
        printf("Received %d bytes: \n", rcv_len);
        for (int i = 0; i < rcv_len; ++i)
            printf("%02X ", (unsigned char)buf[i]);
        printf("\n");

        sem_wait(sem);
        (*counter)++;
        sem_post(sem);
    }
    else if (rcv_len == 0)
    {
        printf("Connection closed by peer.\n");
    }
    else
    {
        perror("sctp_recvmsg");
    }

    close(sockfd);

    return;
}


int main()
{
  // Tạo biến counter trong vùng nhớ chia sẻ
    int *counter = mmap(NULL, sizeof(int),
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS,
                        -1, 0);
    if (counter == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // Khởi tạo counter = 0
    *counter = 0;

    // Tạo semaphore trong vùng nhớ chia sẻ
    sem_t *sem = mmap(NULL, sizeof(sem_t),
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS,
                      -1, 0);
    if (sem == MAP_FAILED) {
        perror("mmap (sem) failed");
        exit(1);
    }

    // Khởi tạo semaphore
    if (sem_init(sem, 1, 1) < 0) { // 1 = shared giữa các process
        perror("sem_init failed");
        exit(1);
    }

    // Tạo 256 tiến trình
    for (int i = 0; i < NUM_PROCESSES; i++) {
                    sleep(1);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            // Tiến trình con
            run(sem, counter);

            // Thoát tiến trình con
            exit(0);
        }
    }

    // Đợi tất cả tiến trình con kết thúc
    for (int i = 0; i < NUM_PROCESSES; i++) {
        wait(NULL);
    }

    // In kết quả cuối cùng
    printf("Final counter value: %d\n", *counter);

    // Hủy semaphore và giải phóng vùng nhớ
    sem_destroy(sem);
    munmap(counter, sizeof(int));
    munmap(sem, sizeof(sem_t));

    return 0;
}
