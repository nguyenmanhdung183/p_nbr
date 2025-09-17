#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"// sua thanh ip cua enb xem
#define SERVER_PORT 36422
#define STREAM_NO 0
#define X2AP_PPID 0

// Convert hex string to binary
int hexstr_to_bin(const char *hexstr, unsigned char *buf, size_t max_len) {
    size_t len = strlen(hexstr);
    if (len % 2 != 0 || len / 2 > max_len) return -1;
    for (size_t i = 0; i < len / 2; ++i)
        sscanf(hexstr + 2 * i, "%2hhx", &buf[i]);
    return len / 2;
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    unsigned char buffer[1024];

    const char *hex_payload =
        "0024004100000100f4003a000002001500080054f24000396a4000fa0027000800020054f240396a4006560054f2400051d60b863300010029400100003740050000000000";

    int payload_len = hexstr_to_bin(hex_payload, buffer, sizeof(buffer));
    if (payload_len < 0) {
        fprintf(stderr, "Invalid hex string\n");
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    int ret = sctp_sendmsg(sockfd, buffer, payload_len,
                           NULL, 0,              
                           0,     
                           0,                 
                           STREAM_NO,          
                           0,                   
                           0);                   

    if (ret < 0) {
        perror("sctp_sendmsg");
        close(sockfd);
        return 1;
    }

    printf(" Sent %d bytes using sctp_sendmsg().\n", ret);
    while(1){}
    close(sockfd);
    return 0;
}
