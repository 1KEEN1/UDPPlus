#include "func.h"

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