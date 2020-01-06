/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

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
  // first add up to 4 terminator bits.
  int usedBits = length * 8 + (8 - bitPos);
  int terminatorBits = 4;
  if (usedBits + terminatorBits > capacity * 8) {
    terminatorBits = capacity * 8 - usedBits;
  }
  write(0, terminatorBits);
  // now pad to next byte
  padToByte();
  bool first = true;
  while (length < capacity) {
    data[length++] = first ? 236 : 17;
    first = !first;
  }
}
