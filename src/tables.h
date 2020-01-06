/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

#pragma once

static const struct {
  ECL ecl;
  int numericMax;
  int alphaMax;
  int byteMax;
  int version;
} modes[] = {
  { ECL::L, 41, 25, 17, 1 },
  { ECL::M, 34, 20, 14, 1 },
  { ECL::Q, 27, 16, 11, 1 },
  { ECL::H, 17, 10, 7, 1 },
  { ECL::L, 77, 47, 32, 2 },
  { ECL::M, 63, 38, 26, 2 },
  { ECL::Q, 48, 29, 20, 2 },
  { ECL::H, 34, 20, 14, 2 },
  { ECL::L, 127, 77, 53, 3 },
  { ECL::M, 101, 61, 42, 3 },
  { ECL::Q, 77, 47, 32, 3 },
  { ECL::H, 58, 35, 24, 3 },
  { ECL::L, 187, 114, 78, 4 },
  { ECL::M, 149, 90, 62, 4 },
  { ECL::Q, 111, 67, 46, 4 },
  { ECL::H, 82, 50, 34, 4 },
  { ECL::L, 255, 154, 106, 5 },
  { ECL::M, 202, 122, 84, 5 },
  { ECL::Q, 144, 87, 60, 5 },
  { ECL::H, 106, 64, 44, 5 },
  { ECL::L, 322, 195, 134, 6 },
  { ECL::M, 255, 154, 106, 6 },
  { ECL::Q, 178, 108, 74, 6 },
  { ECL::H, 139, 84, 58, 6 },
};

#define numModes (sizeof(modes) / sizeof(modes[0]))

static const struct {
  int version;
  ECL ecl;
  int ecPerBlock;
  int g1Blocks;
  int g1DataPerBlock;
  int g2Blocks;
  int g2DataPerBlock;
} ecTable[] = {
  { 1, ECL::L, 7, 1, 19, 0, 0 },
  { 1, ECL::M, 10, 1, 16, 0, 0 },
  { 1, ECL::Q, 13, 1, 13, 0, 0 },
  { 1, ECL::H, 17, 1, 9, 0, 0 },
  { 2, ECL::L, 10, 1, 34, 0, 0 },
  { 2, ECL::M, 16, 1, 28, 0, 0 },
  { 2, ECL::Q, 22, 1, 22, 0, 0 },
  { 2, ECL::H, 28, 1, 16, 0, 0 },
  { 3, ECL::L, 15, 1, 55, 0, 0 },
  { 3, ECL::M, 26, 1, 44, 0, 0 },
  { 3, ECL::Q, 18, 2, 17, 0, 0 },
  { 3, ECL::H, 22, 2, 13, 0, 0 },
  { 4, ECL::L, 20, 1, 80, 0, 0 },
  { 4, ECL::M, 18, 2, 32, 0, 0 },
  { 4, ECL::Q, 26, 2, 24, 0, 0 },
  { 4, ECL::H, 16, 4, 9, 0, 0 },
  { 5, ECL::L, 26, 1, 108, 0, 0 },
  { 5, ECL::M, 24, 2, 43, 0, 0 },
  { 5, ECL::Q, 18, 2, 15, 2, 16 },
  { 5, ECL::H, 22, 2, 11, 2, 12 },
  { 6, ECL::L, 18, 2, 68, 0, 0 },
  { 6, ECL::M, 16, 4, 27, 0, 0 },
  { 6, ECL::Q, 24, 4, 19, 0, 0 },
  { 6, ECL::H, 28, 4, 15, 0, 0 },
};

#define numECs (sizeof(ecTable) / sizeof(ecTable[0]))
