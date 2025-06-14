#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <openssl/ssl.h>

#define CHUNK_SIZE 1400 // Оптимальный размер блока

int main() {
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
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    printf("Server started on port %d\n", SERVER_PORT);

    uint8_t aes_key[AES_KEY_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    FILE *output_file = fopen("received_file.txt", "wb");
    if (!output_file) {
        perror("Failed to open output file");
        close(sockfd);
        return 1;
    }

    while (1) {
        MyTransportHeader pkt;
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        ssize_t recv_len = recvfrom(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len <= 0) {
            perror("recvfrom failed");
            continue;
        }

        uint16_t received_checksum = pkt.checksum;
        pkt.checksum = 0;
        uint16_t computed_checksum = calculate_checksum(&pkt);

        if (received_checksum != computed_checksum) {
            printf("Checksum mismatch!\n");
            continue;
        }

        pkt.checksum = received_checksum;

        if (pkt.flags & FLAG_SYN) {
            printf("Received SYN, sending SYN-ACK...\n");
            MyTransportHeader syn_ack = {0};
            syn_ack.flags = FLAG_SYN | FLAG_ACK;
            syn_ack.seq = htonl(54321);
            syn_ack.ack = ntohl(pkt.seq) + 1;
            syn_ack.enc_type = pkt.enc_type;
            syn_ack.checksum = calculate_checksum(&syn_ack);

            sendto(sockfd, &syn_ack, sizeof(syn_ack), 0, (struct sockaddr *)&client_addr, addr_len);
        } else if (pkt.flags & FLAG_DATA) {
            if (pkt.enc_type == ENCRYPTION_AES) {
                if (decrypt_packet(&pkt, aes_key) != 0) {
                    printf("Failed to decrypt packet\n");
                    continue;
                }
            } else if (pkt.enc_type == ENCRYPTION_XOR) {
                xor_encrypt_decrypt(&pkt);
            }

            fwrite(pkt.no_enc.data, 1, ntohs(pkt.data_len), output_file);
            fflush(output_file);

            MyTransportHeader ack = {0};
            ack.flags = FLAG_ACK;
            ack.ack = ntohl(pkt.seq) + ntohs(pkt.data_len);
            ack.checksum = calculate_checksum(&ack);

            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&client_addr, addr_len);
        } else if (pkt.flags & FLAG_FIN) {
            printf("Connection closed by client\n");
            break;
        }
    }

    fclose(output_file);
    close(sockfd);
    return 0;
}
