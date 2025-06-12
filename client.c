#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define CHUNK_SIZE 1400 // Оптимальный размер блока

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file_path>\n", argv[0]);
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

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Failed to open file");
        close(sockfd);
        return 1;
    }

    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    clock_t start_time = clock();

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        if (sendto(sockfd, buffer, bytes_read, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Failed to send data");
            fclose(file);
            close(sockfd);
            return 1;
        }
    }

    fclose(file);
    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Data transfer time: %f seconds\n", time_spent);

    close(sockfd);
    return 0;
}
