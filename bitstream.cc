#include "bitstream.h"

void BitStream::write(uint32_t value, int bits) {
  value <<= 32 - bits;
  for (int i = 0; i < bits; i++) {
    data[length] |= (value & 0x80000000) >> (24 + bitPos);
    value <<= 1;
    bitPos++;
    if (bitPos == 8) {
      bitPos = 0;
      length++;
    }
  }
}

void BitStream::padToByte() {
  if (bitPos > 0) {
    length++;
  }
  bitPos = 0;
}

void BitStream::padToCapacity(uint32_t capacity) {
  bool first = true;
  while (length < capacity) {
    data[length++] = first ? 236 : 17;
    first = !first;
  }
}
