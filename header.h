#ifndef HEADER_H
#define HEADER_H

#include <arpa/inet.h>
#include <errno.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_PACKET_SIZE 1400
#define SERVER_PORT 8080

typedef struct {
  __be16 source;  // Port of the source
  __be16 dest;    // Port of the destination
  __be32 seq;     // Sequence number
  __be32 ack_seq; // Acknowledgement number of sequence
  uint8_t flags;
  __be16 checksum;
  __be16 data_length;
  uint8_t fragment_number;
  uint8_t data[MAX_PACKET_SIZE];
} Header;

#endif // HEADER_H
