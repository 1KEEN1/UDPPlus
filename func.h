#ifndef FUNC_H
#define FUNC_H

#include "header.h"

uint16_t calculate_checksum(Header *header);
void send_packet(int socket, struct sockaddr_in *dest_addr, Header *header);

#endif // FUNC_H