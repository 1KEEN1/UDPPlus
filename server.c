#include "header.h"
#include "func.h"

int receive_packet(int socket, Header *header, struct sockaddr_in *src_addr) {
  socklen_t addr_len = sizeof(*src_addr);
  ssize_t received_len = recvfrom(socket, header, sizeof(Header), 0, (struct sockaddr *)src_addr, &addr_len);

  if (received_len <= 0) {
    perror("Packet receiving error!");
    return -1;
  }

  if (header->checksum != calculate_checksum(header)) {
    printf("Checksum error!\n");
    return -1;
  }

  return 0;
}

void process_packet(Header *header) {
  uint32_t seq_number = ntohl(header->seq);
  uint8_t fragment_number = header->fragment_number;

  printf("Received packet:\n");
  printf("  Source Port: %d\n", ntohs(header->source));
  printf("  Dest Port: %d\n", ntohs(header->dest));
  printf("  Sequence Number: %d\n", seq_number);
  printf("  Fragment Number: %d\n", fragment_number);
  printf("  Data Length: %d\n", ntohs(header->data_length));
  printf("  Data: %.*s\n", ntohs(header-data_length), header->data);
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

  printf("Server is listening on port %d\n", SERVER_PORT);

  while (1) {
    Header header;
    struct sockaddr_in client_addr;

    if (receive_packet(sockfd, &header, &client_addr) == 0) {
      process_packet(&header);
    }
  }

  close(sockfd);
  return 0;
}
