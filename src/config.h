/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

#pragma once

#include <memory>
#include "json.h"
#include "qrencoder.h"

enum class Style {
  None = 0,
  Dots = 1,
  HDots = 2,
  VDots = 3,
  HVDots = 4,
};

enum class PatternStyle {
  None = 0,
  Rounded = 1,
  Circle = 2,
};

enum Corner {
  TL = 1,
  TR = 2,
  BL = 4,
  BR = 8,
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
  uint32_t iconColor = 0x000000;
  Style style = Style::None;
  PatternStyle pattern = PatternStyle::None;
  uint8_t corners = 0;

 private:
  uint32_t parseColor(const std::string &s);
};
