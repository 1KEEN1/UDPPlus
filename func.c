#include "func.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>

void crypto_init() {
    // Для OpenSSL
}

void xor_encrypt_decrypt(MyTransportHeader *pkt) {
    size_t len = ntohs(pkt->data_len);
    for (size_t i = 0; i < len && i < MAX_PACKET_SIZE; i++) {
        pkt->no_enc.data[i] ^= xor_key[i % XOR_KEY_SIZE];
    }
}

int encrypt_aes(MyTransportHeader *pkt, const uint8_t *key) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return -1;

    RAND_bytes(pkt->aes.iv, AES_IV_SIZE);

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, pkt->aes.iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    int len;
    if (EVP_EncryptUpdate(ctx, pkt->aes.data, &len, pkt->no_enc.data, ntohs(pkt->data_len)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    int final_len;
    if (EVP_EncryptFinal_ex(ctx, pkt->aes.data + len, &final_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    pkt->data_len = htons(len + final_len);
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}

int encrypt_packet(MyTransportHeader *pkt, const uint8_t *aes_key) {
    switch (pkt->enc_type) {
        case ENCRYPTION_AES: return encrypt_aes(pkt, aes_key);
        case ENCRYPTION_XOR: xor_encrypt_decrypt(pkt); return 0;
        case ENCRYPTION_NONE: return 0;
        default: return -1;
    }
}

uint16_t calculate_checksum(const MyTransportHeader *header) {
    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)header;
    size_t len = sizeof(MyTransportHeader);

    for (size_t i = 0; i < len / 2; i++) {
        if (i == 4) continue;
        sum += ntohs(ptr[i]);
    }

    if (len % 2) sum += ((const uint8_t *)ptr)[len - 1];
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return htons(~sum);
}
