/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

#pragma once

#include <string>
#include <cstdint>
#include "message.h"
#include "bitstream.h"

enum ECL {
  L = 0,
  M = 1,
  Q = 2,
  H = 3,
};

enum Encoding {
  Numeric = 1,
  Alpha = 2,
  Byte = 4,
};

struct Block {
  uint8_t *data;
  uint8_t *ec;
  uint32_t dataLen;
};

class QREncoder {
 public:
  QREncoder();
  Message encode(std::string msg, ECL ecl);

 private:
  Encoding determineEncoding(std::string msg);
  int determineVersion(int length, Encoding encoding, ECL ecl);
  ECL determineOptimumECL(ECL ecl, int length, Encoding encoding, int version);
  void determineBlockInfo(int version, ECL ecl, int *ecPerBlock,
                          int *g1Blocks, int *g1DataPerBlock,
                          int *g2Blocks, int *g2DataPerBlock);
  int determineCCILength(int version, Encoding encoding);
  void encodeNumeric(std::string msg, BitStream *stream);
  void encodeAlpha(std::string msg, BitStream *stream);
  void encodeByte(std::string msg, BitStream *stream);
  void reedSolomon(uint8_t *data, int numEC, int numData, uint8_t *ec);
  uint8_t *createGenerator(int numEC);

  uint8_t glog[256];
  uint8_t gexp[512];
};
