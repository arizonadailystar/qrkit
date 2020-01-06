/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

#include "qrgrid.h"
#include "colors.h"
#include <cstring>
#include <cstdlib>
#include <iostream>

Bitmap QRGrid::generate(Message message) {
  Bitmap bmp;
  bmp.size = ((message.version - 1) * 4) + 21;
  bmp.data = new uint8_t[bmp.size * bmp.size];
  memset(bmp.data, Color::Empty, bmp.size * bmp.size);

  addPatterns(&bmp);
  addAlignment(message.version, &bmp);
  addTiming(&bmp);
  reserveAreas(&bmp);
  fillGrid(message.data, message.length, &bmp);
  int mask = findMask(&bmp);
  uint16_t format = getFormatString(message.ecl, mask);
  addFormat(format, &bmp);

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
            (x != 1 && y != 1 && x != 5 && y != 5) ? Color::Pattern :
            Color::BG;
      }
    }
  }
  // add separators
  for (int x = 0; x < bmp->size; x++) {
    if (x < 8 || x > bmp->size - 8) {
      if (x < 8) {
        bmp->data[(bmp->size - 8) * bmp->size + x] = Color::BG;
      }
      bmp->data[7 * bmp->size + x] = Color::BG;
    }
  }
  for (int y = 0; y < bmp->size; y++) {
    if (y < 8 || y > bmp->size - 8) {
      if (y < 8) {
        bmp->data[bmp->size - 8 + y * bmp->size] = Color::BG;
      }
      bmp->data[7 + y * bmp->size] = Color::BG;
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
          (x != 1 && y != 1 && x != 3 && y != 3) ? Color::Align : Color::BG;
    }
  }
}

void QRGrid::addTiming(Bitmap *bmp) {
  int offset = 6 * bmp->size;
  for (int x = 8; x < bmp->size - 8; x++) {
    bmp->data[offset + x] = (x & 1) ? Color::BG : Color::Timing;
  }
  offset = 8 * bmp->size + 6;
  for (int y = 8; y < bmp->size - 8; y++) {
    bmp->data[offset] = (y & 1) ? Color::BG : Color::Timing;
    offset += bmp->size;
  }
}

void QRGrid::reserveAreas(Bitmap *bmp) {
  // dark module
  bmp->data[(bmp->size - 8) * bmp->size + 8] = Color::Reserved;
  // format areas
  for (int x = 0; x < bmp->size; x++) {
    if (x < 9 || x > bmp->size - 9) {
      int offset = 8 * bmp->size + x;
      if (bmp->data[offset] == 0xff) {
        bmp->data[offset] = Color::Reserved;
      }
      offset = x * bmp->size + 8;
      if (bmp->data[offset] == 0xff) {
        bmp->data[offset] = Color::Reserved;
      }
    }
  }
}

void QRGrid::fillGrid(uint8_t *data, uint32_t length, Bitmap *bmp) {
  int bitsLeft = 0;
  uint8_t value = 0;
  int column = 1;
  int direction = -1;  // up
  int x = bmp->size - 2;
  int y = bmp->size - 1;
  for (int bits = 0; bits < length; ) {
    int offset = y * bmp->size + x + column;
    if (bmp->data[offset] == 0xff) {
      bits++;
      if (bitsLeft == 0) {
        bitsLeft = 8;
        value = *data++;
      }
      bmp->data[offset] = (value & 0x80) ? Color::CodeOn : Color::CodeOff;
      value <<= 1;
      bitsLeft--;
    }
    column ^= 1;
    if (column == 1) {
      y += direction;
      if (direction < 0 && y < 0) {
        y = 0;
        direction = 1;
        x -= 2;
        if (x == 5) {  // skip vertical timing
          x--;
        }
      } else if (direction > 0 && y >= bmp->size) {
        y = bmp->size - 1;
        direction = -1;
        x -= 2;
        if (x == 5) {  // skip vertical timing
          x--;
        }
      }
    }
  }
}

int QRGrid::findMask(Bitmap *bmp) {
  int lowestPenalty = -1, lowestMask = 0;

  Bitmap masked;
  masked.data = new uint8_t[bmp->size * bmp->size];
  masked.size = bmp->size;
  for (int i = 0; i < 8; i++) {
    memcpy(masked.data, bmp->data, bmp->size * bmp->size);
    applyMask(i, &masked);
    int score = scoreGrid(&masked);
    if (score < lowestPenalty || lowestPenalty < 0) {
      lowestPenalty = score;
      lowestMask = i;
    }
  }
  delete [] masked.data;
  applyMask(lowestMask, bmp);
  return lowestMask;
}

void QRGrid::applyMask(int pattern, Bitmap *bmp) {
  for (int y = 0; y < bmp->size; y++) {
    for (int x = 0; x < bmp->size; x++) {
      bool flip = false;
      switch (pattern) {
        case 0:
          flip = ((x + y) & 1) == 0;
          break;
        case 1:
          flip = (y & 1) == 0;
          break;
        case 2:
          flip = (x % 3) == 0;
          break;
        case 3:
          flip = ((x + y) % 3) == 0;
          break;
        case 4:
          flip = ((y / 2 + x / 3) & 1) == 0;
          break;
        case 5:
          flip = ((x * y) & 1) + ((x * y) % 3) == 0;
          break;
        case 6:
          flip = ((((x * y) & 1) + ((x * y) % 3)) & 1) == 0;
          break;
        case 7:
          flip = ((((x + y) & 1) + ((x * y) % 3)) & 1) == 0;
          break;
      }
      if (flip) {
        int offset = y * bmp->size + x;
        if (bmp->data[offset] == CodeOn) {
          bmp->data[offset] = CodeOff;
        } else if (bmp->data[offset] == CodeOff) {
          bmp->data[offset] = CodeOn;
        }
      }
    }
  }
}

int QRGrid::scoreGrid(Bitmap *bmp) {
  // flatten grid for score
  for (int i = 0; i < bmp->size * bmp->size; i++) {
    if (bmp->data[i] == Color::BG || bmp->data[i] == Color::CodeOff) {
      bmp->data[i] = 0;
    } else {
      bmp->data[i] = 1;
    }
  }

  return scoreRule1(bmp) + scoreRule2(bmp) + scoreRule3(bmp) + scoreRule4(bmp);
}

int QRGrid::scoreRule1(Bitmap *bmp) {
  int penalty = 0;
  for (int y = 0; y < bmp->size; y++) {
    int run = 1;
    uint8_t prev = 0xff;
    int offset = y * bmp->size;
    for (int x = 0; x < bmp->size; x++) {
      if (bmp->data[offset] == prev) {
        run++;
      } else {
        if (run >= 5) {
          penalty += run - 2;
        }
        run = 1;
        prev = bmp->data[offset];
      }
      offset++;
    }
    if (run >= 5) {
      penalty += run - 2;
    }
  }
  for (int x = 0; x < bmp->size; x++) {
    int run = 1;
    uint8_t prev = 0xff;
    int offset = x;
    for (int y = 0; y < bmp->size; y++) {
      if (bmp->data[offset] == prev) {
        run++;
      } else {
        if (run >= 5) {
          penalty += run - 2;
        }
        run = 1;
        prev = bmp->data[offset];
      }
      offset += bmp->size;
    }
    if (run >= 5) {
      penalty += run - 2;
    }
  }
  return penalty;
}

int QRGrid::scoreRule2(Bitmap *bmp) {
  int penalty = 0;
  for (int y = 0; y < bmp->size - 1; y++) {
    int offset = y * bmp->size;
    for (int x = 0; x < bmp->size - 1; x++) {
      if (bmp->data[offset] == bmp->data[offset + 1] &&
          bmp->data[offset] == bmp->data[offset + bmp->size] &&
          bmp->data[offset] == bmp->data[offset + 1 + bmp->size]) {
        penalty += 3;
      }
      offset++;
    }
  }
  return penalty;
}

int QRGrid::scoreRule3(Bitmap *bmp) {
  uint8_t bad[] = { 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0 };
  uint8_t bad2[] = { 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1 };
  int penalty = scoreRule3a(bmp, bad);
  penalty += scoreRule3a(bmp, bad2);
  return penalty;
}

int QRGrid::scoreRule3a(Bitmap *bmp, uint8_t *pat) {
  int penalty = 0;
  for (int y = 0; y < bmp->size; y++) {
    int offset = y * bmp->size;
    int match = 0;
    for (int x = 0; x < bmp->size; x++) {
      if (bmp->data[offset] == pat[match]) {
        match++;
      } else {
        offset -= match;
        x -= match;
        match = 0;
      }
      if (match == 11) {
        penalty += 40;
        match = 0;
      }
      offset++;
    }
    if (match == 11) {
      penalty += 40;
    }
  }
  for (int x = 0; x < bmp->size; x++) {
    int offset = x;
    int match = 0;
    for (int y = 0; y < bmp->size; y++) {
      if (bmp->data[offset] == pat[match]) {
        match++;
      } else {
        offset -= match * bmp->size;
        y -= match;
        match = 0;
      }
      if (match == 11) {
        penalty += 40;
        match = 0;
      }
      offset += bmp->size;
    }
    if (match == 11) {
      penalty += 40;
    }
  }
  return penalty;
}

int QRGrid::scoreRule4(Bitmap *bmp) {
  int dark = 0;
  for (int y = 0; y < bmp->size; y++) {
    int offset = y * bmp->size;
    for (int x = 0; x < bmp->size; x++) {
      if (bmp->data[offset++] == 1) {
        dark++;
      }
    }
  }
  dark = (dark * 100) / (bmp->size * bmp->size);
  int prevFive = dark / 5;
  prevFive *= 5;
  int nextFive = prevFive + 5;
  prevFive = abs(prevFive - 50) / 5;
  nextFive = abs(nextFive - 50) / 5;
  if (prevFive < nextFive) {
    return prevFive * 10;
  }
  return nextFive * 10;
}

static uint16_t formatStrings[] = {
  0x5412, 0x5125, 0x5e7c, 0x5b4b, 0x4579, 0x40ce, 0x4797, 0x4aa0,  // m
  0x77c4, 0x72f3, 0x7daa, 0x789d, 0x662f, 0x6318, 0x6c41, 0x6976,  // l
  0x1689, 0x13be, 0x1ce7, 0x19d0, 0x0762, 0x0255, 0x0d0c, 0x083b,  // h
  0x355f, 0x3068, 0x3f31, 0x3a06, 0x24b4, 0x2183, 0x2eda, 0x2bed,  // q
};

static uint8_t eclFormat[] = { 1, 0, 3, 2 };

uint16_t QRGrid::getFormatString(int ecl, int mask) {
  return formatStrings[(eclFormat[ecl] << 3) | mask];
}

void QRGrid::addFormat(uint16_t format, Bitmap *bmp) {
  format <<= 1;  // start with first bit in pos 15
  int xoffset = bmp->size * 8;
  int yoffset = 8;
  for (int i = 0; i < 15; i++) {
    uint8_t color = (format & 0x8000) ? Color::CodeOn : Color::CodeOff;
    format <<= 1;
    if (i < 6) {
      bmp->data[xoffset + i] = color;
    } else if (i == 6) {
      bmp->data[xoffset + i + 1] = color;
    } else {
      bmp->data[xoffset + i + bmp->size - 15] = color;
    }
    if (i < 7) {
      bmp->data[yoffset + ((bmp->size - 1) - i) * bmp->size] = color;
    } else if (i < 9) {
      bmp->data[yoffset + (15 - i) * bmp->size] = color;
    } else {
      bmp->data[yoffset + (14 - i) * bmp->size] = color;
    }
  }
  bmp->data[(bmp->size - 8) * bmp->size + 8] = Color::CodeOn;  // on-block
}
