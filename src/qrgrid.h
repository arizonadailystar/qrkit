/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

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
  void reserveAreas(Bitmap *bitmap);
  void fillGrid(uint8_t *data, uint32_t len, Bitmap *bitmap);
  int findMask(Bitmap *bitmap);
  void applyMask(int pattern, Bitmap *bitmap);
  int scoreGrid(Bitmap *bitmap);
  int scoreRule1(Bitmap *bitmap);
  int scoreRule2(Bitmap *bitmap);
  int scoreRule3(Bitmap *bitmap);
  int scoreRule3a(Bitmap *bitmap, uint8_t *pattern);
  int scoreRule4(Bitmap *bitmap);
  uint16_t getFormatString(int ecl, int mask);
  void addFormat(uint16_t format, Bitmap *bitmap);
};
