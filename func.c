#include "func.h"
#include <arpa/inet.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Initialize crypto
void crypto_init() {
    OpenSSL_add_all_algorithms();
}

// AES-128-CBC Encryption
int encrypt_aes(MyTransportHeader *pkt, const uint8_t *key) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    RAND_bytes(pkt->aes.iv, AES_IV_SIZE);

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, pkt->aes.iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    int len;
    int ciphertext_len = 0;
    uint8_t *data_ptr = pkt->aes.data;

    if (EVP_EncryptUpdate(ctx, data_ptr, &len, data_ptr, ntohs(pkt->data_len)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;

    if (EVP_EncryptFinal_ex(ctx, data_ptr + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    ciphertext_len += len;

    pkt->data_len = htons(ciphertext_len);
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

// AES-128-CBC Decryption
int decrypt_aes(MyTransportHeader *pkt, const uint8_t *key) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, pkt->aes.iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    int len;
    int plaintext_len = 0;
    uint8_t *data_ptr = pkt->aes.data;

    if (EVP_DecryptUpdate(ctx, data_ptr, &len, data_ptr, ntohs(pkt->data_len)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len += len;

    if (EVP_DecryptFinal_ex(ctx, data_ptr + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    plaintext_len += len;

    pkt->data_len = htons(plaintext_len);
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

// XOR Encryption/Decryption
void xor_encrypt_decrypt(MyTransportHeader *pkt) {
    for (size_t i = 0; i < ntohs(pkt->data_len); i++) {
        pkt->no_enc.data[i] ^= xor_key[i % XOR_KEY_SIZE];
    }
}

int encrypt_packet(MyTransportHeader *pkt, const uint8_t *aes_key) {
    switch (pkt->enc_type) {
        case ENCRYPTION_AES:
            return encrypt_aes(pkt, aes_key);
        case ENCRYPTION_XOR:
            xor_encrypt_decrypt(pkt);
            return 0;
        case ENCRYPTION_NONE:
            return 0;
        default:
            return -1;
    }
}

int decrypt_packet(MyTransportHeader *pkt, const uint8_t *aes_key) {
    switch (pkt->enc_type) {
        case ENCRYPTION_AES:
            return decrypt_aes(pkt, aes_key);
        case ENCRYPTION_XOR:
            xor_encrypt_decrypt(pkt);
            return 0;
        case ENCRYPTION_NONE:
            return 0;
        default:
            return -1;
    }
}

uint16_t calculate_checksum(const MyTransportHeader *header) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)header;
    size_t len = sizeof(MyTransportHeader);

    for (size_t i = 0; i < len / 2; i++) {
        if (i == 4) continue; // Skip checksum field
        sum += ntohs(ptr[i]);
    }

    if (len % 2) {
        sum += ((const uint8_t *)ptr)[len - 1];
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return htons(~sum);
}

int establish_connection(int sockfd, struct sockaddr_in *addr, int enc_type) {
    MyTransportHeader syn_pkt = {0};
    syn_pkt.flags = FLAG_SYN;
    syn_pkt.seq = htonl(12345);
    syn_pkt.enc_type = enc_type;
    syn_pkt.checksum = calculate_checksum(&syn_pkt);

    if (sendto(sockfd, &syn_pkt, sizeof(syn_pkt), 0, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("Failed to send SYN");
        return -1;
    }

    MyTransportHeader syn_ack;
    socklen_t addr_len = sizeof(*addr);
    if (recvfrom(sockfd, &syn_ack, sizeof(syn_ack), 0, (struct sockaddr *)addr, &addr_len) <= 0) {
        perror("Failed to receive SYN-ACK");
        return -1;
    }

    uint16_t received_checksum = syn_ack.checksum;
    syn_ack.checksum = 0;
    if (received_checksum != calculate_checksum(&syn_ack)) {
        printf("SYN-ACK checksum mismatch!\n");
        return -1;
    }

    MyTransportHeader ack_pkt = {0};
    ack_pkt.flags = FLAG_ACK;
    ack_pkt.ack = syn_ack.seq + 1;
    ack_pkt.enc_type = enc_type;
    ack_pkt.checksum = calculate_checksum(&ack_pkt);

    if (sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("Failed to send ACK");
        return -1;
    }

    return 0;
}

int send_reliable_data(int sockfd, struct sockaddr_in *addr, const void *data, size_t len, int enc_type, const uint8_t *aes_key) {
    MyTransportHeader pkt = {0};
    pkt.flags = FLAG_DATA;
    pkt.seq = htonl(12346);
    pkt.data_len = htons(len);
    pkt.enc_type = enc_type;
    memcpy(pkt.no_enc.data, data, len);

    if (enc_type == ENCRYPTION_AES) {
        if (encrypt_packet(&pkt, aes_key) != 0) {
            perror("Failed to encrypt packet");
            return -1;
        }
    } else if (enc_type == ENCRYPTION_XOR) {
        xor_encrypt_decrypt(&pkt);
    }

    pkt.checksum = calculate_checksum(&pkt);

    if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("Failed to send data");
        return -1;
    }

    MyTransportHeader ack;
    socklen_t addr_len = sizeof(*addr);
    if (recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)addr, &addr_len) <= 0) {
        perror("Failed to receive ACK");
        return -1;
    }

    uint16_t received_checksum = ack.checksum;
    ack.checksum = 0;
    if (received_checksum != calculate_checksum(&ack)) {
        printf("ACK checksum mismatch!\n");
        return -1;
    }

    return 0;
}

void close_connection(int sockfd, struct sockaddr_in *addr) {
    MyTransportHeader fin_pkt = {0};
    fin_pkt.flags = FLAG_FIN;
    fin_pkt.checksum = calculate_checksum(&fin_pkt);

    sendto(sockfd, &fin_pkt, sizeof(fin_pkt), 0, (struct sockaddr *)addr, sizeof(*addr));
}
