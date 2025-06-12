#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define WINDOW_SIZE 32

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    // Увеличиваем буферы
    int buf_size = 1024 * 1024;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    FILE *file = fopen("received.bin", "wb");
    uint8_t aes_key[AES_KEY_SIZE] = {0};

    uint32_t expected_seq = 0;
    char *window[WINDOW_SIZE] = {NULL};

    while (1) {
        MyTransportHeader pkt;
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        ssize_t recv_len = recvfrom(sockfd, &pkt, sizeof(pkt), 0,
                                   (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len <= 0) continue;

        uint32_t seq_num = ntohl(pkt.seq);
        size_t data_len = ntohs(pkt.data_len);

        // Быстрая обработка данных
        if (pkt.flags & FLAG_DATA) {
            if (seq_num >= expected_seq && seq_num < expected_seq + WINDOW_SIZE) {
                if (window[seq_num % WINDOW_SIZE] == NULL) {
                    window[seq_num % WINDOW_SIZE] = malloc(data_len);
                    memcpy(window[seq_num % WINDOW_SIZE], pkt.no_enc.data, data_len);
                }

                // Пакетный ACK
                if (seq_num == expected_seq) {
                    while (window[expected_seq % WINDOW_SIZE] != NULL) {
                        fwrite(window[expected_seq % WINDOW_SIZE], 1, data_len, file);
                        free(window[expected_seq % WINDOW_SIZE]);
                        window[expected_seq % WINDOW_SIZE] = NULL;
                        expected_seq++;
                    }

                    MyTransportHeader ack = {0};
                    ack.flags = FLAG_ACK;
                    ack.ack = htonl(expected_seq);
                    sendto(sockfd, &ack, sizeof(ack), 0,
                          (struct sockaddr *)&client_addr, addr_len);
                }
            }
        }
    }

    fclose(file);
    close(sockfd);
    return 0;
}
