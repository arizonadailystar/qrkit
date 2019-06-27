#pragma once

#include "message.h"

struct Bitmap {
  uint8_t *data = nullptr;
  int size = 0;
};

class QRGrid {
 public:
  Bitmap generate(Message message);

 private:
  void addPatterns(Bitmap *bitmap);
  void addAlignment(int version, Bitmap *bitmap);
  void addTiming(Bitmap *bitmap);
};
