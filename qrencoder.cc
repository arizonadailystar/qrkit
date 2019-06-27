#include "qrencoder.h"
#include "tables.h"
#include <cstring>
#include <iostream>

QREncoder::QREncoder() {
  for (int i = 0; i < 256; i++) {
    if (i == 0) {
      exp2num[i] = 1;
    } else {
      int val = exp2num[i - 1] * 2;
      if (val > 255) {
        val ^= 285;
      }
      exp2num[i] = val;
    }
    if (i != 255) {
      num2exp[exp2num[i]] = i;
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
  // temp disabled
//  ecl = determineOptimumECL(ecl, msg.length(), encoding, version);

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
  stream.padToByte();
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

  message.data = new uint8_t[message.length];
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
  for (int i = 0; i < numBlocks; i++) {
    delete [] blocks[i].data;
    delete [] blocks[i].ec;
  }

  message.version = version;

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
    if (number < 100) {
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
  const uint8_t *generator = exponents[numEC];
  uint8_t terms[numData];
  for (int i = 0; i < numData; i++) {
    terms[i] = data[i];
  }

  for (int cycle = 0; cycle < numData; cycle++) {
    uint8_t term = num2exp[terms[0]];
    for (int i = 0; i < numData - 1; i++) {
      uint8_t val = i < numEC ? exp2num[(generator[i] + term) % 255] : 0;
      terms[i] = terms[i + 1] ^ val;
    }
    terms[numData - 1] = 0;
  }
  for (int i = 0; i < numEC; i++){
    ec[i] = terms[i];
  }
}
