#include "header.h"

uint16_t calculate_checksum(Header *header) {
  uint32_t sum = 0;
  uint16_t *ptr = (uint16_t *)header;
  size_t header_len = sizeof(Header) / 2;

  for (size_t i = 0; i < header_len; i++) {
    sum += *ptr++;
  }

  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  return ~sum;
}

void send_packet(int socket, struct sockaddr_in *dest_addr, Header *header) {
  header->checksum = calculate_checksum(header);
  sendto(socket, header, sizeof(Header), 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr));
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
  // Initialization data of the server
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERVER_PORT);
  servaddr.sin_addr.s_addr = INADDR_ANY; // For now it's any address

  // Creating the Header of the packet
  Header header;
  memset(&header, 0, sizeof(header));
  header.source = htons(12345); // Test port
  header.dest = htons(SERVER_PORT);
  header.seq = htonl(1); // First packet
  header.ack_seq = htonl(0);
  header.flags = 0;
  header.data_length = htons(5);
  header.fragment_number = 0;
  memcpy(header.data, "Hello", 5); // Data

  //Sending the packet
  send_packet(sockfd, &servaddr, &header);

  printf("Packet sent to server.\n");

  close(sockfd);
  return 0;
}
