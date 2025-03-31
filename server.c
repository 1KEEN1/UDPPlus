#include "func.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("Socket error");
    return 1;
  }

  struct sockaddr_in serv_addr = {0};
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("Bind failed");
    close(sockfd);
    return 1;
  }

  printf("Server started on port %d\n", SERVER_PORT);

  while (1) {
    MyTransportHeader pkt;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Recieve packet
    ssize_t recv_len = recvfrom(sockfd, &pkt, sizeof(pkt), 0,
                                (struct sockaddr *)&client_addr, &addr_len);

    if (recv_len <= 0) {
      perror("recvfrom failed");
      continue;
    }

    // Check for checksum
    uint16_t received_checksum = pkt.checksum;
    pkt.checksum = 0;
    uint16_t computed_checksum = calculate_checksum(&pkt);

    if (received_checksum != computed_checksum) {
      printf("Checksum mismatch! (got: 0x%04X, expected: 0x%04X)\n",
             received_checksum, computed_checksum);
      continue;
    }

    pkt.checksum = received_checksum;

    // Handle flags
    if (pkt.flags & FLAG_SYN) {
      printf("Received SYN, sending SYN-ACK...\n");
      MyTransportHeader syn_ack = {0};
      syn_ack.flags = FLAG_SYN | FLAG_ACK;
      syn_ack.seq = htonl(54321);
      syn_ack.ack = pkt.seq + 1;
      syn_ack.checksum = calculate_checksum(&syn_ack);

      sendto(sockfd, &syn_ack, sizeof(syn_ack), 0,
             (struct sockaddr *)&client_addr, addr_len);
    } else if (pkt.flags & FLAG_DATA) {
      printf("Received data: %.*s\n", ntohs(pkt.data_len), pkt.data);

      // Send ACK
      MyTransportHeader ack = {0};
      ack.flags = FLAG_ACK;
      ack.ack = pkt.seq + ntohs(pkt.data_len);
      ack.checksum = calculate_checksum(&ack);

      sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&client_addr,
             addr_len);
    } else if (pkt.flags & FLAG_FIN) {
      printf("Connection closed by client\n");
    }
  }

  close(sockfd);
  return 0;
}