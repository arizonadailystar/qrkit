/** @copyright 2019 Sean Kasun */

#pragma once

#include <memory>
#include "json.h"
#include "qrencoder.h"

enum Style {
  None = 0,
  Dots = 1,
  HDots = 2,
  VDots = 3,
  HVDots = 4,
};

class Config {
 public:
  Config(std::shared_ptr<JSONData> json);

  ECL minECL = ECL::L;
  uint32_t padding = 10;
  uint32_t scale = 10;
  uint32_t backgroundColor = 0xffffff;
  uint32_t patternColor = 0x000000;
  uint32_t alignColor = 0x000000;
  uint32_t codeColor = 0x000000;
  Style style = Style::None;

 private:
  uint32_t parseColor(const std::string &s);
};
