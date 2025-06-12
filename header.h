#ifndef HEADER_H
#define HEADER_H

#include <netinet/in.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <time.h>

#define SERVER_PORT 8080
#define MAX_PACKET_SIZE 1400

// Encryption types
#define ENCRYPTION_NONE 0
#define ENCRYPTION_AES 1
#define ENCRYPTION_XOR 2

// AES settings
#define AES_KEY_SIZE 16
#define AES_IV_SIZE 16

// XOR key
#define XOR_KEY_SIZE 4
static const uint8_t xor_key[XOR_KEY_SIZE] = {0xAA, 0xBB, 0xCC, 0xDD};

// Flags
#define FLAG_SYN 0x01
#define FLAG_ACK 0x02
#define FLAG_FIN 0x04
#define FLAG_DATA 0x08

typedef struct {
    uint32_t seq;
    uint32_t ack;
    uint16_t checksum;
    uint8_t flags;
    uint8_t enc_type; // Encryption type
    uint16_t data_len;
    union {
        struct {
            uint8_t iv[AES_IV_SIZE]; // AES IV
            uint8_t data[MAX_PACKET_SIZE];
        } aes;
        struct {
            uint8_t data[MAX_PACKET_SIZE]; // XOR / No encryption
        } no_enc;
    };
} MyTransportHeader;

#endif // HEADER_H
