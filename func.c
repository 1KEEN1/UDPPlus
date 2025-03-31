#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

uint16_t calculate_checksum(const MyTransportHeader *header) {
  uint32_t sum = 0;
  const uint16_t *ptr = (const uint16_t *)header;
  size_t len = sizeof(MyTransportHeader);

  // Skip checksum (so that it will not affect on calculations)
  for (size_t i = 0; i < len / 2; i++) {
    if (i == 4)
      continue; // Skip checksum
    sum += ptr[i];
  }

  if (len % 2) {
    sum += ((const uint8_t *)ptr)[len - 1];
  }

  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  return ~(uint16_t)sum;
}

int establish_connection(int sockfd, struct sockaddr_in *addr) {
  MyTransportHeader syn_pkt = {0};
  syn_pkt.flags = FLAG_SYN;
  syn_pkt.seq = htonl(12345); // First seq

  if (sendto(sockfd, &syn_pkt, sizeof(syn_pkt), 0, (struct sockaddr *)addr,
             sizeof(*addr)) < 0) {
    perror("Failed to send SYN");
    return -1;
  }

  // Wait SYN-ACK
  MyTransportHeader syn_ack;
  socklen_t addr_len = sizeof(*addr);
  if (recvfrom(sockfd, &syn_ack, sizeof(syn_ack), 0, (struct sockaddr *)addr,
               &addr_len) <= 0) {
    perror("Failed to receive SYN-ACK");
    return -1;
  }

  // Send ACK
  MyTransportHeader ack_pkt = {0};
  ack_pkt.flags = FLAG_ACK;
  ack_pkt.ack = syn_ack.seq + 1;

  if (sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, (struct sockaddr *)addr,
             sizeof(*addr)) < 0) {
    perror("Failed to send ACK");
    return -1;
  }

  return 0;
}

int send_reliable_data(int sockfd, struct sockaddr_in *addr, const void *data,
                       size_t len) {
  MyTransportHeader pkt = {0};
  pkt.flags = FLAG_DATA;
  pkt.seq = htonl(12346); // Number of packet
  pkt.data_len = htons(len);
  memcpy(pkt.data, data, len);
  pkt.checksum = calculate_checksum(&pkt);

  if (sendto(sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)addr,
             sizeof(*addr)) < 0) {
    perror("Failed to send data");
    return -1;
  }

  // Wait ACK
  MyTransportHeader ack;
  socklen_t addr_len = sizeof(*addr);
  if (recvfrom(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)addr,
               &addr_len) <= 0) {
    perror("Failed to receive ACK");
    return -1;
  }

  return 0;
}

void close_connection(int sockfd, struct sockaddr_in *addr) {
  MyTransportHeader fin_pkt = {0};
  fin_pkt.flags = FLAG_FIN;

  sendto(sockfd, &fin_pkt, sizeof(fin_pkt), 0, (struct sockaddr *)addr,
         sizeof(*addr));
}