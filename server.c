#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define CHUNK_SIZE 1400 // Оптимальный размер блока

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        return 1;
    }

    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    printf("Server started on port %d\n", SERVER_PORT);

    FILE *output_file = fopen("received_file.txt", "wb");
    if (!output_file) {
        perror("Failed to open output file");
        close(sockfd);
        return 1;
    }

    char buffer[CHUNK_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (1) {
        ssize_t recv_len = recvfrom(sockfd, buffer, CHUNK_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len <= 0) {
            perror("recvfrom failed");
            continue;
        }

        fwrite(buffer, 1, recv_len, output_file);
    }

    fclose(output_file);
    close(sockfd);
    return 0;
}
