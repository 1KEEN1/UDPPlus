#ifndef FUNC_H
#define FUNC_H

#include "header.h"

// Initialize crypto
void crypto_init();

// Encryption/Decryption functions
int encrypt_packet(MyTransportHeader *pkt, const uint8_t *aes_key);
int decrypt_packet(MyTransportHeader *pkt, const uint8_t *aes_key);
void xor_encrypt_decrypt(MyTransportHeader *pkt);

uint16_t calculate_checksum(const MyTransportHeader *header);
int establish_connection(int sockfd, struct sockaddr_in *addr, int enc_type);
int send_reliable_data(int sockfd, struct sockaddr_in *addr, const void *data, size_t len, int enc_type, const uint8_t *aes_key);
void close_connection(int sockfd, struct sockaddr_in *addr);

#endif // FUNC_H
