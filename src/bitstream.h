/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

#pragma once

#include <cstdint>

class BitStream {
 public:
  uint32_t length = 0;
  uint8_t *data = nullptr;

  void write(uint32_t value, int bits);
  void padToByte();
  void padToCapacity(uint32_t capacity);

 private:
  int bitPos = 0;
};
