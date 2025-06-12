#ifndef FUNC_H
#define FUNC_H

#include "header.h"

// Crypto functions
void crypto_init();
int encrypt_packet(MyTransportHeader *pkt, const uint8_t *aes_key);
int decrypt_packet(MyTransportHeader *pkt, const uint8_t *aes_key);
void xor_encrypt_decrypt(MyTransportHeader *pkt); // Добавлена декларация

// Protocol functions
uint16_t calculate_checksum(const MyTransportHeader *header);

#endif // FUNC_H
