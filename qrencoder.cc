/** @copyright 2019 Sean Kasun */

#include "qrencoder.h"
#include "tables.h"
#include <cstring>
#include <iostream>

QREncoder::QREncoder() {
  uint8_t pinit = 0, p1 = 1, p2 = 0, p3 = 0,
          p4 = 0, p5 = 0, p6 = 0, p7 = 0, p8 = 0;
  gexp[0] = 1;
  gexp[255] = 1;
  glog[0] = 0;
  for (int i = 1; i < 256; i++) {
    pinit = p8;
    p8 = p7;
    p7 = p6;
    p6 = p5;
    p5 = p4 ^ pinit;
    p4 = p3 ^ pinit;
    p3 = p2 ^ pinit;
    p2 = p1;
    p1 = pinit;
    gexp[i] = p1 + p2 * 2 + p3 * 4 + p4 * 8 + p5 * 16 + p6 * 32 + p7 * 64 +
        p8 * 128;
    gexp[i + 255] = gexp[i];
  }
  for (int i = 1; i < 256; i++) {
    for (int z = 0; z < 256; z++) {
      if (gexp[z] == i) {
        glog[i] = z;
        break;
      }
    }
  }
}

Message QREncoder::encode(std::string msg, ECL ecl) {
  Message message;

  Encoding encoding = determineEncoding(msg);
  int version = determineVersion(msg.length(), encoding, ecl);
  if (version < 0) {
    std::cerr << "Message is too long.  We don't support larger QR codes"
              << std::endl;
    return message;
  }
  ecl = determineOptimumECL(ecl, msg.length(), encoding, version);

  int ecPerBlock = 0;
  int g1Blocks = 0;
  int g1DataPerBlock = 0;
  int g2Blocks = 0;
  int g2DataPerBlock = 0;
  determineBlockInfo(version, ecl, &ecPerBlock, &g1Blocks, &g1DataPerBlock,
                     &g2Blocks, &g2DataPerBlock);

  BitStream stream;
  int totalData = g1Blocks * g1DataPerBlock + g2Blocks + g2DataPerBlock;
  stream.data = new uint8_t[totalData];

  // add mode indicator
  stream.write(encoding, 4);
  stream.write(msg.length(), determineCCILength(version, encoding));
  switch (encoding) {
    case Encoding::Numeric:
      encodeNumeric(msg, &stream);
      break;
    case Encoding::Alpha:
      encodeAlpha(msg, &stream);
      break;
    case Encoding::Byte:
      encodeByte(msg, &stream);
      break;
  }
  stream.padToCapacity(totalData);

  int numBlocks = g1Blocks + g2Blocks;
  Block *blocks = new Block[numBlocks];
  for (int i = 0; i < g1Blocks; i++) {
    blocks[i].dataLen = g1DataPerBlock;
    blocks[i].data = new uint8_t[g1DataPerBlock];
    memcpy(blocks[i].data, stream.data + i * g1DataPerBlock, g1DataPerBlock);
    blocks[i].ec = new uint8_t[ecPerBlock];
  }
  for (int i = 0; i < g2Blocks; i++) {
    blocks[g1Blocks + i].dataLen = g2DataPerBlock;
    blocks[g1Blocks + i].data = new uint8_t[g2DataPerBlock];
    memcpy(blocks[g1Blocks + i].data, stream.data +
           g1Blocks * g1DataPerBlock + i * g2DataPerBlock, g2DataPerBlock);
    blocks[g1Blocks + i].ec = new uint8_t[ecPerBlock];
  }

  delete [] stream.data;

  message.length = 0;
  for (int i = 0; i < g1Blocks + g2Blocks; i++) {
    reedSolomon(blocks[i].data, ecPerBlock, blocks[i].dataLen, blocks[i].ec);
    message.length += blocks[i].dataLen + ecPerBlock;
  }

  message.data = new uint8_t[message.length + 1];
  int dataLen = g1DataPerBlock > g2DataPerBlock ? g1DataPerBlock :
      g2DataPerBlock;
  int pos = 0;
  for (int i = 0; i < dataLen; i++) {
    for (int j = 0; j < numBlocks; j++) {
      if (i < blocks[j].dataLen) {
        message.data[pos++] = blocks[j].data[i];
      }
    }
  }
  for (int i = 0; i < ecPerBlock; i++) {
    for (int j = 0; j < numBlocks; j++) {
      message.data[pos++] = blocks[j].ec[i];
    }
  }
  message.data[pos++] = 0;  // pad with a 0

  for (int i = 0; i < numBlocks; i++) {
    delete [] blocks[i].data;
    delete [] blocks[i].ec;
  }
  message.length *= 8;

  message.version = version;
  message.ecl = ecl;
  if (version > 1) {
    message.length += 7;  // 7 padding bits
  }

  return message;
}

Encoding QREncoder::determineEncoding(std::string msg) {
  bool canNumeric = true, canAlpha = true;
  for (int i = 0; i < msg.length(); i++) {
    if (msg[i] < '0' || msg[i] > '9') {
      canNumeric = false;
    }
    if ((msg[i] < '0' || msg[i] > '9') &&
        (msg[i] < 'A' || msg[i] > 'Z') &&
        strchr(" $%*+-./:", msg[i]) == nullptr) {
      canAlpha = false;
    }
  }
  return canNumeric ? Encoding::Numeric :
      canAlpha ? Encoding::Alpha : Encoding::Byte;
}

int QREncoder::determineVersion(int length, Encoding encoding, ECL ecl) {
  for (int i = 0; i < numModes; i++) {
    if (modes[i].ecl == ecl &&
        ((encoding == Encoding::Numeric && length <= modes[i].numericMax) ||
         (encoding == Encoding::Alpha && length <= modes[i].alphaMax) ||
         (encoding == Encoding::Byte && length <= modes[i].byteMax))) {
      return modes[i].version;
    }
  }
  return -1;
}

ECL QREncoder::determineOptimumECL(ECL ecl, int length, Encoding encoding,
                                   int version) {
  for (int i = 0; i < numModes; i++) {
    if (modes[i].version == version && modes[i].ecl > ecl &&
        ((encoding == Encoding::Numeric && length <= modes[i].numericMax) ||
         (encoding == Encoding::Alpha && length <= modes[i].alphaMax) ||
         (encoding == Encoding::Byte && length <= modes[i].byteMax))) {
      ecl = modes[i].ecl;
    }
  }
  return ecl;
}

void QREncoder::determineBlockInfo(int version, ECL ecl, int *ecPerBlock,
                                   int *g1Blocks, int *g1DataPerBlock,
                                   int *g2Blocks, int *g2DataPerBlock) {
  for (int i = 0; i < numECs; i++) {
    if (ecTable[i].version == version && ecTable[i].ecl == ecl) {
      *ecPerBlock = ecTable[i].ecPerBlock;
      *g1Blocks = ecTable[i].g1Blocks;
      *g1DataPerBlock = ecTable[i].g1DataPerBlock;
      *g2Blocks = ecTable[i].g2Blocks;
      *g2DataPerBlock = ecTable[i].g2DataPerBlock;
      break;
    }
  }
}

int QREncoder::determineCCILength(int version, Encoding encoding) {
  switch (encoding) {
    case Encoding::Numeric:
      return 10;
    case Encoding::Alpha:
      return 9;
    case Encoding::Byte:
      return 8;
  }
  return 8;
}

void QREncoder::encodeNumeric(std::string msg, BitStream *stream) {
  for (int i = 0; i < msg.length(); i += 3) {
    int number = 0;
    for (int j = 0; j < 3 && i + j < msg.length(); j++) {
      number *= 10;
      number += msg[i + j] - '0';
    }
    if (number < 100 && i + 3 >= msg.length()) {
      if (number < 10) {
        stream->write(number, 4);
      } else {
        stream->write(number, 7);
      }
    } else {
      stream->write(number, 10);
    }
  }
}

void QREncoder::encodeAlpha(std::string msg, BitStream *stream) {
  static const char *special = " $%*+-./:";

  for (int i = 0; i < msg.length(); i += 2) {
    int code = 0;
    for (int j = 0; j < 2 && i + j < msg.length(); j++) {
      code *= 45;
      if (msg[i + j] >= '0' && msg[i + j] <= '9') {
        code += msg[i + j] - '0';
      } else if (msg[i + j] >= 'A' && msg[i + j] <= 'Z') {
        code += msg[i + j] - 'A' + 10;
      } else {
        code += strchr(special, msg[i + j]) - special + 36;
      }
    }
    if (i == msg.length() - 1) {
      stream->write(code, 6);
    } else {
      stream->write(code, 11);
    }
  }
}

void QREncoder::encodeByte(std::string msg, BitStream *stream) {
  for (int i = 0; i < msg.length(); i++) {
    stream->write(msg[i], 8);
  }
}

void QREncoder::reedSolomon(uint8_t *data, int numEC, int numData,
                            uint8_t *ec) {
  uint8_t *generator = createGenerator(numEC);
  uint8_t terms[numEC + 1];
  memset(terms, 0, numEC + 1);

  for (int i = 0; i < numData; i++) {
    uint8_t term = data[i] ^ terms[numEC - 1];
    for (int j = numEC - 1; j > 0; j--) {
      uint8_t val = 0;
      if (generator[j] && term) {
        val = gexp[glog[generator[j]] + glog[term]];
      }
      terms[j] = terms[j - 1] ^ val;
    }
    terms[0] = 0;
    if (generator[0] && term) {
      terms[0] = gexp[glog[generator[0]] + glog[term]];
    }
  }
  delete [] generator;
  int curEC = 0;
  for (int i = numEC - 1; i >= 0; i--) {
    ec[curEC++] = terms[i];
  }
}

uint8_t *QREncoder::createGenerator(int numEC) {
  uint8_t *poly = new uint8_t[numEC * 2];
  memset(poly, 0, numEC * 2);
  poly[0] = 1;

  uint8_t *q = new uint8_t[numEC * 2];
  uint8_t *temp = new uint8_t[numEC * 4];
  uint8_t *dest = new uint8_t[numEC * 2];

  for (int i = 0; i < numEC; i++) {
    memset(q, 0, numEC * 2);
    q[0] = gexp[i];
    q[1] = 1;

    memset(dest, 0, numEC * 2);
    for (int j = 0; j < numEC * 2; j++) {
      memset(temp + numEC * 2, 0, numEC * 2);
      for (int k = 0; k < numEC * 2; k++) {
        if (poly[k] && q[j]) {
          temp[k] = gexp[glog[poly[k]] + glog[q[j]]];
        } else {
          temp[k] = 0;
        }
      }
      for (int k = numEC * 4 - 1; k >= j; k--) {
        temp[k] = temp[k - j];
      }
      memset(temp, 0, j);
      for (int k = 0; k < numEC * 2; k++) {
        dest[k] ^= temp[k];
      }
    }
    memcpy(poly, dest, numEC * 2);
  }
  delete [] q;
  delete [] temp;
  delete [] dest;
  return poly;
}
