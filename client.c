#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <openssl/ssl.h>

#define CHUNK_SIZE 1400 // Оптимальный размер блока

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <encryption_type> <file_path>\n", argv[0]);
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

    // Инициализация OpenSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

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

    FILE *file = fopen(argv[2], "rb");
    if (!file) {
        perror("Failed to open file");
        close(sockfd);
        return 1;
    }

    uint8_t aes_key[AES_KEY_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    clock_t start_time = clock();

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        if (send_reliable_data(sockfd, &serv_addr, buffer, bytes_read, enc_type, aes_key) < 0) {
            fclose(file);
            close(sockfd);
            return 1;
        }
    }

    fclose(file);
    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    printf("Data transfer time: %f seconds\n", time_spent);

    close_connection(sockfd, &serv_addr);
    close(sockfd);
    return 0;
}
