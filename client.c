#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <encryption_type>\n", argv[0]);
        printf("Encryption types:\n");
        printf("0: No encryption\n");
        printf("1: AES-128-CBC\n");
        printf("2: XOR\n");
        return 1;
    }

    int enc_type = atoi(argv[1]);
    if (enc_type < 0 || enc_type > 2) {
        printf("Invalid encryption type\n");
        return 1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        return 1;
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (establish_connection(sockfd, &serv_addr, enc_type) < 0) {
        close(sockfd);
        return 1;
    }

    const char *message = "Hello, custom protocol!";
    clock_t start_time = clock();

    if (send_reliable_data(sockfd, &serv_addr, message, strlen(message), enc_type, NULL) < 0) {
        close(sockfd);
        return 1;
    }

    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Data transfer time: %f seconds\n", time_spent);

    close_connection(sockfd, &serv_addr);
    close(sockfd);
    return 0;
}
