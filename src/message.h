/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

#pragma once

#include <cstdint>

struct Message {
  uint32_t length = 0;
  uint8_t *data = nullptr;
  int version = 0;
  uint8_t ecl = 0;
};
