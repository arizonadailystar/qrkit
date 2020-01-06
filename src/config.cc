/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

#include "config.h"
#include <iostream>
#include <algorithm>

Config::Config(std::shared_ptr<JSONData> json) {
  if (json == nullptr) {
    std::cerr << "No config loaded" << std::endl;
    std::cerr << "Using defaults..." << std::endl;
    return;
  }
  if (json->has("minECL")) {
    auto ecl = json->at("minECL")->asString();
    switch (tolower(ecl[0])) {
      case 'l':
        minECL = ECL::L;
        break;
      case 'm':
        minECL = ECL::M;
        break;
      case 'q':
        minECL = ECL::Q;
        break;
      case 'h':
        minECL = ECL::H;
        break;
      default:
        std::cerr << "minECL must be one of L, M, Q, or H" << std::endl;
        break;
    }
  }
  if (json->has("padding")) {
    padding = json->at("padding")->asNumber();
  }
  if (json->has("scale")) {
    scale = json->at("scale")->asNumber();
  }
  if (json->has("background")) {
    backgroundColor = parseColor(json->at("background")->asString());
  }
  if (json->has("pattern")) {
    patternColor = parseColor(json->at("pattern")->asString());
  }
  if (json->has("align")) {
    alignColor = parseColor(json->at("align")->asString());
  }
  if (json->has("code")) {
    codeColor = parseColor(json->at("code")->asString());
  }
  if (json->has("icon")) {
    iconColor = parseColor(json->at("icon")->asString());
  }
  if (json->has("style")) {
    std::string style = json->at("style")->asString();
    std::transform(style.begin(), style.end(), style.begin(), ::tolower);
    if (style == "none") {
      this->style = Style::None;
    } else if (style == "dots") {
      this->style = Style::Dots;
    } else if (style == "hdots") {
      this->style = Style::HDots;
    } else if (style == "vdots") {
      this->style = Style::VDots;
    } else if (style == "hvdots") {
      this->style = Style::HVDots;
    } else {
      std::cerr << "Style must be one of 'none', 'dots', 'hdots', 'vdots',"
               " or 'hhvdots'" << std::endl;
    }
  }
  if (json->has("patstyle")) {
    std::string style = json->at("patstyle")->asString();
    std::transform(style.begin(), style.end(), style.begin(), ::tolower);
    if (style == "none") {
      pattern = PatternStyle::None;
    } else if (style == "rounded") {
      pattern = PatternStyle::Rounded;
    } else if (style == "circle") {
      pattern = PatternStyle::Circle;
    } else {
      std::cerr << "Pattern Style must be one of 'none', 'rounded',"
                " or 'circle'" << std::endl;
    }
  }
  if (pattern == PatternStyle::Rounded && json->has("corners")) {
    auto array = json->at("corners");
    for (int i = 0; i < array->length(); i++) {
      std::string c = array->at(i)->asString();
      std::transform(c.begin(), c.end(), c.begin(), ::tolower);
      if (c == "tl") {
        corners |= Corner::TL;
      } else if (c == "tr") {
        corners |= Corner::TR;
      } else if (c == "bl") {
        corners |= Corner::BL;
      } else if (c == "br") {
        corners |= Corner::BR;
      } else {
        std::cerr << "Corners must be 'tl', 'tr', 'bl', or 'br'" << std::endl;
      }
    }
  }
}

uint32_t Config::parseColor(const std::string &s) {
  uint32_t c = 0;
  for (int i = 0; i < s.length(); i++) {
    if (s[i] == '#') {
      continue;
    }
    c <<= 4;
    if (s[i] >= '0' && s[i] <= '9') {
      c |= s[i] - '0';
    } else if (s[i] >= 'A' && s[i] <= 'F') {
      c |= s[i] - 'A' + 10;
    } else if (s[i] >= 'a' && s[i] <= 'f') {
      c |= s[i] - 'a' + 10;
    } else {
      std::cerr << "Invalid color format: " << s << std::endl;
    }
  }
  return c;
}
