#include "header.h"
#include "func.h"

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
