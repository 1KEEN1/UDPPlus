#include "header.h"

uint16_t calculate_checksum(Header *header) {
  uint32_t sum = 0;
  uint16_t *ptr = (uint16_t *)header;
  size_t header_len = sizeof(Header) / 2;

  for (size_t i = 0; i < header_len; i++) {
    sum += *ptr++;
  }

  // Collapsing the sum so that doesn't be over the limit
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  // Inverting the sum to increase the probability of detecting errors.
  return ~sum;
}

int main() {
  // Initialization of RAW socket for the implementation over UDP
  int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
  if (sockfd < 0) {
    perror("Socket initialization error!");
    return 1;
  }

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));
  // Initialize data of the server
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERVER_PORT);
  servaddr.sin_addr.s_addr = INADDR_ANY;

  // Bind the socket to the server address
  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("Bind FAILED!");
    close(sockfd);
    return 1;
  }

  return 0;
}
