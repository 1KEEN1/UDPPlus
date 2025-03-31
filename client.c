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
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // Connection
  MyTransportHeader syn_pkt = {0};
  syn_pkt.flags = FLAG_SYN;
  syn_pkt.seq = htonl(12345);
  syn_pkt.checksum = calculate_checksum(&syn_pkt);

  if (sendto(sockfd, &syn_pkt, sizeof(syn_pkt), 0,
             (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("Failed to send SYN");
    close(sockfd);
    return 1;
  }

  // Wait SYN-ACK
  MyTransportHeader syn_ack;
  socklen_t addr_len = sizeof(serv_addr);
  if (recvfrom(sockfd, &syn_ack, sizeof(syn_ack), 0,
               (struct sockaddr *)&serv_addr, &addr_len) <= 0) {
    perror("Failed to receive SYN-ACK");
    close(sockfd);
    return 1;
  }

  // Check for checksum
  uint16_t received_checksum = syn_ack.checksum;
  syn_ack.checksum = 0;
  if (received_checksum != calculate_checksum(&syn_ack)) {
    printf("SYN-ACK checksum mismatch!\n");
    close(sockfd);
    return 1;
  }
  syn_ack.checksum = received_checksum;

  // Send ACK
  MyTransportHeader ack_pkt = {0};
  ack_pkt.flags = FLAG_ACK;
  ack_pkt.ack = syn_ack.seq + 1;
  ack_pkt.checksum = calculate_checksum(&ack_pkt);

  if (sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0,
             (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("Failed to send ACK");
    close(sockfd);
    return 1;
  }

  // Send data
  const char *message = "Hello, custom protocol!";
  MyTransportHeader data_pkt = {0};
  data_pkt.flags = FLAG_DATA;
  data_pkt.seq = htonl(12346);
  data_pkt.data_len = htons(strlen(message));
  memcpy(data_pkt.data, message, strlen(message));
  data_pkt.checksum = calculate_checksum(&data_pkt);

  if (sendto(sockfd, &data_pkt, sizeof(data_pkt), 0,
             (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("Failed to send data");
    close(sockfd);
    return 1;
  }

  // Wait for ACK
  MyTransportHeader data_ack;
  if (recvfrom(sockfd, &data_ack, sizeof(data_ack), 0,
               (struct sockaddr *)&serv_addr, &addr_len) <= 0) {
    perror("Failed to receive DATA ACK");
    close(sockfd);
    return 1;
  }

  // Check for checksum
  received_checksum = data_ack.checksum;
  data_ack.checksum = 0;
  if (received_checksum != calculate_checksum(&data_ack)) {
    printf("DATA ACK checksum mismatch!\n");
    close(sockfd);
    return 1;
  }

  printf("Data sent and acknowledged!\n");

  // Close the connection
  MyTransportHeader fin_pkt = {0};
  fin_pkt.flags = FLAG_FIN;
  fin_pkt.checksum = calculate_checksum(&fin_pkt);

  sendto(sockfd, &fin_pkt, sizeof(fin_pkt), 0, (struct sockaddr *)&serv_addr,
         sizeof(serv_addr));

  close(sockfd);
  return 0;
}