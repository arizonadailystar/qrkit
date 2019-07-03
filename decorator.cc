/** @copyright 2019 Sean Kasun */

#include "decorator.h"
#include <setjmp.h>
#include <png.h>
#include <cstdlib>
#include <iostream>

void Decorator::decorate(const Bitmap &bitmap, const Config &config,
                         const char *filename) {

  int width = config.scale * bitmap.size + config.padding * 2;
  int height = config.scale * bitmap.size + config.padding * 2;
  uint8_t *pixels = new uint8_t[width * 4 * height];

  // first apply background color
  int offset = 0;
  for (int i = 0; i < width * height; i++) {
    pixels[offset++] = config.backgroundColor >> 16;
    pixels[offset++] = (config.backgroundColor >> 8) & 0xff;
    pixels[offset++] = config.backgroundColor & 0xff;
    pixels[offset++] = 0xff;  // alpha
  }

  for (int y = 0; y < bitmap.size; y++) {
    offset = y * bitmap.size;
    int outOffset = (y * config.scale + config.padding) * width * 4 +
        config.padding * 4;
    for (int x = 0; x < bitmap.size; x++) {
      if (bitmap.data[offset] != Color::BG &&
          bitmap.data[offset] != Color::CodeOff) {
        uint8_t mask = 0xf;
        uint32_t color = getColor(static_cast<Color>(bitmap.data[offset]),
                                  config);
        drawDot(pixels + outOffset, width * 4, color, config.scale, mask);
      }
      outOffset += config.scale * 4;
      offset++;
    }
  }

  auto png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                         nullptr, nullptr);
  auto info_ptr = png_create_info_struct(png_ptr);
  if (setjmp(png_jmpbuf(png_ptr))) {
    std::cerr << "PNG Failure" << std::endl;
    return;
  }
  FILE *f = fopen(filename, "wb");
  if (!f) {
    std::cerr << "Failed to create " << filename << std::endl;
    return;
  }
  png_init_io(png_ptr, f);

  png_bytep row_pointers[height];
  for (int row = 0; row < height; row++) {
    row_pointers[row] = pixels + row * 4 * width;
  }
  png_set_IHDR(png_ptr, info_ptr, width, height,
               8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);
  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, nullptr);
  fclose(f);
}

uint32_t Decorator::getColor(Color color, const Config &config) {
  switch (color) {
    case Color::Pattern:
      return config.patternColor;
    case Color::Align:
      return config.alignColor;
    case Color::Timing:
    case Color::Reserved:
    case Color::CodeOn:
      return config.codeColor;
    default:
      return config.backgroundColor;
  }
}

void Decorator::drawDot(uint8_t *out, int stride, uint32_t color,
                        uint32_t scale, uint8_t mask) {
  for (int y = 0; y < scale; y++) {
    int offset = y * stride;
    for (int x = 0; x < scale; x++) {
      out[offset++] = color >> 16;
      out[offset++] = (color >> 8) & 0xff;
      out[offset++] = color & 0xff;
      out[offset++] = 0xff;
    }
  }
}
