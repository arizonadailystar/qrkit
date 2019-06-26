#pragma once

#include <string>
#include <cstdint>
#include "message.h"

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

class QREncoder {
 public:
  Message encode(std::string msg, ECL ecl);

 private:
  Encoding determineEncoding(std::string msg);
  int determineVersion(int length, Encoding encoding, ECL ecl);
  ECL determineOptimumECL(ECL ecl, int length, Encoding encoding, int version);
  void determineBlockInfo(int version, ECL ecl, int *ecPerBlock,
                          int *g1Blocks, int *g1DataPerBlock,
                          int *g2Blocks, int *g2DataPerBlock);
  int determineCCILength(int version, Encoding encoding);
  void encodeNumeric(std::string msg, Message *message);
  void encodeAlpha(std::string msg, Message *message);
  void encodeByte(std::string msg, Message *message);
};
