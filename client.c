#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define WINDOW_SIZE 32
#define CHUNK_SIZE (1400 - sizeof(MyTransportHeader)) // Безопасный размер

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

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct timeval tv = {.tv_usec = 100000};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    int fd = open(argv[2], O_RDONLY);
    struct stat st;
    fstat(fd, &st);
    size_t file_size = st.st_size;
    char *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);

    uint8_t aes_key[AES_KEY_SIZE] = {0}; // В реальном коде используйте настоящий ключ

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t base = 0;
    size_t next_seq = 0;
    size_t last_seq = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;

    while (base < last_seq) {
        while (next_seq < base + WINDOW_SIZE && next_seq < last_seq) {
            size_t offset = next_seq * CHUNK_SIZE;
            size_t chunk_size = (next_seq == last_seq - 1) ? 
                file_size - offset : CHUNK_SIZE;

            // Проверка переполнения буфера
            if (chunk_size > MAX_PACKET_SIZE - sizeof(MyTransportHeader)) {
                fprintf(stderr, "Chunk size too large\n");
                break;
            }

            MyTransportHeader pkt = {0};
            pkt.flags = FLAG_DATA;
            pkt.seq = htonl(next_seq);
            pkt.enc_type = enc_type;
            pkt.data_len = htons(chunk_size);
            
            memcpy(pkt.no_enc.data, file_data + offset, chunk_size);

            if (enc_type == ENCRYPTION_AES) {
                if (encrypt_packet(&pkt, aes_key) < 0) break;
            } 
            else if (enc_type == ENCRYPTION_XOR) {
                xor_encrypt_decrypt(&pkt);
            }

            sendto(sockfd, &pkt, sizeof(pkt), 0, 
                  (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            
            next_seq++;
        }

        MyTransportHeader ack;
        socklen_t addr_len = sizeof(serv_addr);
        if (recvfrom(sockfd, &ack, sizeof(ack), 0, 
                    (struct sockaddr *)&serv_addr, &addr_len) > 0) {
            if (ack.flags & FLAG_ACK) {
                uint32_t ack_seq = ntohl(ack.ack);
                if (ack_seq > base) base = ack_seq;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Transfer completed in %.3f seconds (%.2f MB/s)\n", 
           time_taken, (file_size / (1024.0 * 1024.0)) / time_taken);

    munmap(file_data, file_size);
    close(fd);
    close(sockfd);
    return 0;
}
