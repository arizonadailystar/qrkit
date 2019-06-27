#include "qrgrid.h"
#include <cstring>

Bitmap QRGrid::generate(Message message) {
  Bitmap bmp;
  bmp.size = ((message.version - 1) * 4) + 21;
  bmp.data = new uint8_t[bmp.size * bmp.size];
  memset(bmp.data, 0, bmp.size * bmp.size);

  addPatterns(&bmp);
  addAlignment(message.version, &bmp);
  addTiming(&bmp);

  return bmp;
}

void QRGrid::addPatterns(Bitmap *bmp) {
  int corners[] = {
    0, 0,
    bmp->size - 7, 0,
    0, bmp->size - 7,
  };
  for (int i = 0; i < 3; i++) {
    int start = corners[i * 2] + corners[i * 2 + 1] * bmp->size;
    for (int y = 0; y < 7; y++) {
      int offset = y * bmp->size + start;
      for (int x = 0; x < 7; x++) {
        bmp->data[offset++] = y == 0 || y == 6 || x == 0 || x == 6 ||
            (x != 1 && y != 1 && x != 5 && y != 5) ? 1 : 0;
      }
    }
  }
}

static const int alignxy[][2] = {
  {},  // version 1
  { 18, 18, },  // version 2
  { 22, 22, },  // version 3
  { 26, 26, },  // version 4
  { 30, 30, },  // version 5
  { 34, 34, },  // version 6
};

void QRGrid::addAlignment(int version, Bitmap *bmp) {
  if (version == 1) {
    return;
  }
  int start = alignxy[version - 1][0] + alignxy[version - 1][1] * bmp->size;
  start -= 2 + 2 * bmp->size;
  for (int y = 0; y < 5; y++) {
    int offset = start + y * bmp->size;
    for (int x = 0; x < 5; x++) {
      bmp->data[offset++] = x == 0 || y == 0 || x == 4 || y == 4 ||
          (x != 1 && y != 1 && x != 3 && y != 3) ? 1 : 0;
    }
  }
}

void QRGrid::addTiming(Bitmap *bmp) {
  int offset = 6 * bmp->size;
  for (int x = 8; x < bmp->size - 8; x++) {
    bmp->data[offset + x] = (x & 1) ? 0 : 1;
  }
  offset = 8 * bmp->size + 6;
  for (int y = 8; y < bmp->size - 8; y++) {
    bmp->data[offset] = (y & 1) ? 0 : 1;
    offset += bmp->size;
  }
}
