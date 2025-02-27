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
