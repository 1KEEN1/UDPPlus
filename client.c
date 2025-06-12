#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

#define WINDOW_SIZE 32       // Размер окна для sliding window
#define CHUNK_SIZE (64 * 1024) // 64KB chunks

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <0|1|2> <file_path>\n", argv[0]);
        return 1;
    }

    int enc_type = atoi(argv[1]);
    if (enc_type < 0 || enc_type > 2) {
        printf("Invalid encryption type\n");
        return 1;
    }

    // Настройка сокета с таймаутом
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100ms timeout
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    // Memory-map файл для быстрого чтения
    FILE *file = fopen(argv[2], "rb");
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fileno(file), 0);

    uint8_t aes_key[AES_KEY_SIZE] = {0}; // Используйте реальный ключ

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Sliding window реализация
    size_t base = 0;
    size_t next_seq = 0;
    size_t last_seq = file_size / CHUNK_SIZE;

    while (base < last_seq) {
        // Отправка пакетов в окне
        while (next_seq < base + WINDOW_SIZE && next_seq <= last_seq) {
            size_t offset = next_seq * CHUNK_SIZE;
            size_t chunk_size = (next_seq == last_seq) ? 
                file_size % CHUNK_SIZE : CHUNK_SIZE;

            MyTransportHeader pkt = {0};
            pkt.flags = FLAG_DATA;
            pkt.seq = htonl(next_seq);
            pkt.data_len = htons(chunk_size);
            memcpy(pkt.no_enc.data, file_data + offset, chunk_size);

            if (enc_type == ENCRYPTION_AES) {
                encrypt_packet(&pkt, aes_key);
            } else if (enc_type == ENCRYPTION_XOR) {
                xor_encrypt_decrypt(&pkt);
            }

            sendto(sockfd, &pkt, sizeof(pkt), 0, 
                  (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            
            next_seq++;
        }

        // Получение ACK
        MyTransportHeader ack;
        socklen_t addr_len = sizeof(serv_addr);
        if (recvfrom(sockfd, &ack, sizeof(ack), 0, 
                    (struct sockaddr *)&serv_addr, &addr_len) > 0) {
            if (ack.flags & FLAG_ACK) {
                uint32_t ack_seq = ntohl(ack.ack);
                if (ack_seq > base) {
                    base = ack_seq;
                }
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Transfer completed in %.3f seconds (%.2f MB/s)\n", 
           time_taken, (file_size / (1024 * 1024)) / time_taken);

    munmap(file_data, file_size);
    fclose(file);
    close(sockfd);
    return 0;
}
