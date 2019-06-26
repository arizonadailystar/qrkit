#pragma once

#include <cstdint>

class Message {
 public:
  uint32_t length = 0;
  uint8_t *data = nullptr;

 public:
  void write(uint32_t value, int bits);
  void padToByte();
  void padToCapacity(uint32_t capacity);

 private:
  int bitPos = 0;
};
