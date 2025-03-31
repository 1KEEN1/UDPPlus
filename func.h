#ifndef FUNC_H
#define FUNC_H

#include "header.h"

// 3-way handshake
int establish_connection(int sockfd, struct sockaddr_in *addr);

int send_reliable_data(int sockfd, struct sockaddr_in *addr, const void *data,
                       size_t len);

void close_connection(int sockfd, struct sockaddr_in *addr);

uint16_t calculate_checksum(const MyTransportHeader *header);

#endif // FUNC_H