#ifndef HEADER_H
#define HEADER_H

#include <netinet/in.h>
#include <stdint.h>

#define SERVER_PORT 8080
#define MAX_PACKET_SIZE 1400

// Flags
#define FLAG_SYN 0x01  // Begin connection
#define FLAG_ACK 0x02  // Acknowledgment
#define FLAG_FIN 0x04  // Finish
#define FLAG_DATA 0x08 // Data

typedef struct {
  uint32_t seq;
  uint32_t ack;
  uint16_t checksum;
  uint8_t flags;
  uint16_t data_len;
  uint8_t data[MAX_PACKET_SIZE];
} MyTransportHeader;

#endif // HEADER_H