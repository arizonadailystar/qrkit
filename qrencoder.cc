#include "qrencoder.h"
#include "modes.h"
#include <cstring>
#include <iostream>

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

  int ecPerBlock, g1Blocks, g1DataPerBlock, g2Blocks, g2DataPerBlock;
  determineBlockInfo(version, ecl, &ecPerBlock, &g1Blocks, &g1DataPerBlock,
                     &g2Blocks, &g2DataPerBlock);

  message.data = new uint8_t[g1Blocks * g1DataPerBlock +
      g2Blocks * g2DataPerBlock];

  // add mode indicator
  message.write(encoding, 4);
  message.write(msg.length(), determineCCILength(version, encoding));
  switch (encoding) {
    case Encoding::Numeric:
      encodeNumeric(msg, &message);
      break;
    case Encoding::Alpha:
      encodeAlpha(msg, &message);
      break;
    case Encoding::Byte:
      encodeByte(msg, &message);
      break;
  }
  message.padToByte();
  message.padToCapacity(g1Blocks * g1DataPerBlock + g2Blocks * g2DataPerBlock);

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
  if (version < 9) {
    switch (encoding) {
      case Encoding::Numeric:
        return 10;
      case Encoding::Alpha:
        return 9;
      case Encoding::Byte:
        return 8;
    }
  }
  return 8;
}

void QREncoder::encodeNumeric(std::string msg, Message *message) {
  for (int i = 0; i < msg.length(); i += 3) {
    int number = 0;
    for (int j = 0; j < 3 && i + j < msg.length(); j++) {
      number *= 10;
      number += msg[i + j] - '0';
    }
    if (number < 100) {
      if (number < 10) {
        message->write(number, 4);
      } else {
        message->write(number, 7);
      }
    } else {
      message->write(number, 10);
    }
  }
}

void QREncoder::encodeAlpha(std::string msg, Message *message) {
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
      message->write(code, 6);
    } else {
      message->write(code, 11);
    }
  }
}

void QREncoder::encodeByte(std::string msg, Message *message) {
  for (int i = 0; i < msg.length(); i++) {
    message->write(msg[i], 8);
  }
}
