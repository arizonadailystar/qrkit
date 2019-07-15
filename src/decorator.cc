/** @copyright 2019 Sean Kasun */

#include "decorator.h"
#include <setjmp.h>
#include <png.h>
#include <cstdlib>
#include <iostream>
#include <cmath>

void Decorator::decorate(const Bitmap &bitmap, const Config &config,
                         const char *embed, const char *filename) {

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

  offset = 0;
  for (int y = 0; y < bitmap.size; y++) {
    int outOffset = (y * config.scale + config.padding) * width * 4 +
        config.padding * 4;
    for (int x = 0; x < bitmap.size; x++) {
      if (bitmap.data[offset] != Color::BG &&
          bitmap.data[offset] != Color::Empty &&
          bitmap.data[offset] != Color::Pattern &&
          bitmap.data[offset] != Color::CodeOff) {
        uint8_t mask = 0x0;
        uint32_t color = getColor(bitmap.data[offset], config);
        // check above
        if (y > 0 &&
            getColor(bitmap.data[offset - bitmap.size], config) == color) {
          mask |= 0x1;
        }
        // check left
        if (x > 0 && getColor(bitmap.data[offset - 1], config) == color) {
          mask |= 0x2;
        }
        // check below
        if (y < bitmap.size - 1 &&
            getColor(bitmap.data[offset + bitmap.size], config) == color) {
          mask |= 0x4;
        }
        // check right
        if (x < bitmap.size - 1 &&
            getColor(bitmap.data[offset + 1], config) == color) {
          mask |= 0x8;
        }
        switch (config.style) {
          case Style::None:
            mask |= 0xf;  // all connected
            break;
          case Style::Dots:
            mask &= 0x0;  // all disconnected
            break;
          case Style::HDots:
            mask &= 0xa;  // no vertical connections
            break;
          case Style::VDots:
            mask &= 0x5;  // no horizontal connections
            break;
          case Style::HVDots:  // blend everything
            break;
        }
        drawDot(pixels + outOffset, width * 4, color, config.backgroundColor,
                config.scale, mask);
      }
      outOffset += config.scale * 4;
      offset++;
    }
  }
  // add corners.
  drawPattern(pixels + config.padding * width * 4 + config.padding * 4,
              width * 4, config.patternColor, config.backgroundColor,
              config.scale, config.pattern, config.corners);
  drawPattern(pixels + config.padding * width * 4 + config.padding * 4 +
              (bitmap.size - 7) * config.scale * 4, width * 4,
              config.patternColor, config.backgroundColor,
              config.scale, config.pattern, config.corners);
  drawPattern(pixels + ((bitmap.size - 7) * config.scale + config.padding) *
              width * 4 + config.padding * 4, width * 4,
              config.patternColor, config.backgroundColor,
              config.scale, config.pattern, config.corners);


  if (embed != nullptr) {
    embedIcon(embed, pixels + (8 * config.scale + config.padding) * width * 4 +
              (8 * config.scale + config.padding) * 4, width * 4,
              config.iconColor, config.backgroundColor,
              (bitmap.size - 16) * config.scale);
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

uint32_t Decorator::getColor(uint8_t color, const Config &config) {
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
                        uint32_t background, uint32_t scale, uint8_t mask) {
  double radius = scale / 2.0;
  double r2 = radius * radius;
  for (int y = 0; y < scale; y++) {
    int offset = y * stride;
    double dy = y - radius;
    for (int x = 0; x < scale; x++) {
      double dx = x - radius;
      double dist = dx * dx + dy * dy;
      bool skip = false;
      if (((mask & 1) && y < radius) || ((mask & 2) && x < radius) ||
          ((mask & 4) && y >= radius) || ((mask & 8) && x >= radius)) {
        skip = true;
      }

      if (dist < r2 || skip) {
        uint32_t newcolor = color;
        if (!skip && sqrt(r2) - sqrt(dist) <= 1) {  // antialias
          newcolor = blend(color, background, sqrt(r2) - sqrt(dist));
        }
        out[offset++] = newcolor >> 16;
        out[offset++] = (newcolor >> 8) & 0xff;
        out[offset++] = newcolor & 0xff;
        out[offset++] = 0xff;
      } else {
        offset += 4;
      }
    }
  }
}

void Decorator::drawPattern(uint8_t *out, int stride, uint32_t color,
                            uint32_t background, uint32_t scale,
                            PatternStyle style, uint8_t corners) {
  switch (style) {
    case PatternStyle::None:
      drawSquare(out, stride, color, background, scale);
      break;
    case PatternStyle::Rounded:
      drawRounded(out, stride, color, background, scale, corners);
      break;
    case PatternStyle::Circle:
      drawCircle(out, stride, color, background, scale);
      break;
  }
}

void Decorator::drawSquare(uint8_t *out, int stride, uint32_t color,
                           uint32_t background, uint32_t scale) {
  double center = (scale * 7.0) / 2.0;
  for (int y = 0; y < 7 * scale; y++) {
    int offset = y * stride;
    double dy = fabs(y - center);
    for (int x = 0; x < 7 * scale; x++) {
      double dx = fabs(x - center);
      if ((dx < scale * 1.5 && dy < scale * 1.5) ||
          dx >= scale * 2.5 || dy >= scale * 2.5) {
        out[offset++] = color >> 16;
        out[offset++] = (color >> 8) & 0xff;
        out[offset++] = color & 0xff ;
        out[offset++] = 0xff;
      } else {
        offset += 4;
      }
    }
  }
}

void Decorator::drawRounded(uint8_t *out, int stride, uint32_t color,
                           uint32_t background, uint32_t scale,
                           uint8_t corners) {
  double radius = (scale - 1) * (scale - 1);
  double radius2 = (scale + 1) * 2 * (scale + 1) * 2;
  double center = (scale * 7.0) / 2.0;
  double cx = 0.0, cy = 0.0;

  for (int y = 0; y < 7 * scale; y++) {
    int offset = y * stride;
    double dy = fabs(y - center);
    for (int x = 0; x < 7 * scale; x++) {
      double dx = fabs(x - center);
      bool plot = false;
      bool round = false;
      bool arc = false;
      uint32_t newcolor = color;

      if ((dx < scale * 1.5 && dy < scale * 1.5) ||
          dx >= scale * 2.5 || dy >= scale * 2.5) {
        plot = true;
        // if 4 corners of dot.
        if (x >= 2 * scale && x < 3 * scale &&
            y >= 2 * scale && y < 3 * scale && (corners & Corner::TL)) {
          round = true;
          cx = 3 * scale - x;
          cy = 3 * scale - y;
        } else if (x >= 2 * scale && x < 3 * scale &&
                   y >= 4 * scale && y < 5 * scale && (corners & Corner::BL)) {
          round = true;
          cx = 3 * scale - x;
          cy = y - 4 * scale;
        } else if (x >= 4 * scale && x < 5 * scale &&
                   y >= 2 * scale && y < 3 * scale && (corners & Corner::TR)) {
          round = true;
          cx = x - 4 * scale;
          cy = 3 * scale - y;
        } else if (x >= 4 * scale && x < 5 * scale &&
                   y >= 4 * scale && y < 5 * scale && (corners & Corner::BR)) {
          round = true;
          cx = x - 4 * scale;
          cy = y - 4 * scale;
        }
      }
      if (x < 2 * scale && y < 2 * scale && (corners & Corner::TL)) {
        arc = true;
        cx = 2 * scale - x;
        cy = 2 * scale - y;
      } else if (x < 2 * scale && y > 5 * scale && (corners & Corner::BL)) {
        arc = true;
        cx = 2 * scale - x;
        cy = y - 5 * scale;
      } else if (x > 5 * scale && y < 2 * scale && (corners & Corner::TR)) {
        arc = true;
        cx = x - 5 * scale;
        cy = 2 * scale - y;
      } else if (x > 5 * scale && y > 5 * scale && (corners & Corner::BR)) {
        arc = true;
        cx = x - 5 * scale;
        cy = y - 5 * scale;
      }
      if (round) {
        plot = false;
        double dist = cx * cx + cy * cy;
        if (dist < radius) {
          plot = true;
          if (sqrt(radius) - sqrt(dist) <= 1) {
            newcolor = blend(color, background, sqrt(radius) - sqrt(dist));
          }
        }
      }
      if (arc) {
        plot = false;
        double dist = cx * cx + cy * cy;
        if (dist < radius2 && dist >= radius - 0.5) {
          plot = true;
          if (sqrt(radius2) - sqrt(dist) <= 1) {
            newcolor = blend(color, background, sqrt(radius2) - sqrt(dist));
          } else if (sqrt(dist) - sqrt(radius) <= 1) {
            newcolor = blend(color, background, sqrt(dist) - sqrt(radius));
          }
        }
      }
      if (plot) {
        out[offset++] = newcolor >> 16;
        out[offset++] = (newcolor >> 8) & 0xff;
        out[offset++] = newcolor & 0xff;
        out[offset++] = 0xff;
      } else {
        offset += 4;
      }
    }
  }
}

void Decorator::drawCircle(uint8_t *out, int stride, uint32_t color,
                           uint32_t background, uint32_t scale) {
  uint32_t ringOuterRadius = (scale * 7) / 2;
  uint32_t ringInnerRadius = (scale * 5) / 2;
  uint32_t dotRadius = (scale * 3) / 2;
  double ro2 = ringOuterRadius * ringOuterRadius;
  double ri2 = ringInnerRadius * ringInnerRadius;
  double dr2 = dotRadius * dotRadius;
  for (int y = 0; y < 7 * scale; y++) {
    int offset = y * stride;
    int dy = y - ringOuterRadius;
    for (int x = 0; x < 7 * scale; x++) {
      int dx = x - ringOuterRadius;
      double dist = dx * dx + dy * dy;
      if ((dist < ro2 && dist >= ri2) || dist < dr2) {
        uint32_t newcolor = color;
        if (dist < dr2 && sqrt(dr2) - sqrt(dist) <= 1) {
          newcolor = blend(color, background, sqrt(dr2) - sqrt(dist));
        } else if (dist >= ri2 && sqrt(dist) - sqrt(ri2) <= 1) {
          newcolor = blend(color, background, sqrt(dist) - sqrt(ri2));
        } else if (sqrt(ro2) - sqrt(dist) <= 1) {
          newcolor = blend(color, background, sqrt(ro2) - sqrt(dist));
        }
        out[offset++] = newcolor >> 16;
        out[offset++] = (newcolor >> 8) & 0xff;
        out[offset++] = newcolor & 0xff;
        out[offset++] = 0xff;
      } else {
        offset += 4;
      }
    }
  }
}

void Decorator::rgb2hsv(double r, double g, double b,
                        double *h, double *s, double *v) {
  double min = r < g ? r : g;
  min = min < b ? min : b;
  double max = r > g ? r : g;
  max = max > b ? max : b;
  *v = max;
  double delta = max - min;
  if (delta < 0.00001) {
    *s = 0;
    *h = 0;
    return;
  }
  if (max > 0.0) {
    *s = delta / max;
  } else {
    *s = 0.0;
    *h = NAN;
    return;
  }
  if (r >= max) {
    *h = (g - b) / delta;
  } else if (g >= max) {
    *h = 2.0 + (b - r) / delta;
  } else {
    *h = 4.0 + (r - g) / delta;
  }
  *h *= 60.0;

  if (*h < 0.0) {
    *h += 360.0;
  }
}

void Decorator::hsv2rgb(double h, double s, double v,
                        double *r, double *g, double *b) {
  if (s <= 0.0) {
    *r = v;
    *g = v;
    *b = v;
    return;
  }
  double hh = h;
  if (hh >= 360) {
    hh = 0.0;
  }
  hh /= 60.0;
  int32_t i = hh;
  double ff = hh - i;
  double p = v * (1 - s);
  double q = v * (1 - (s * ff));
  double t = v * (1 - (s * (1 - ff)));
  switch (i) {
    case 0:
      *r = v;
      *g = t;
      *b = p;
      break;
    case 1:
      *r = q;
      *g = v;
      *b = p;
      break;
    case 2:
      *r = p;
      *g = v;
      *b = t;
      break;
    case 3:
      *r = p;
      *g = q;
      *b = v;
      break;
    case 4:
      *r = t;
      *g = p;
      *b = v;
      break;
    default:
      *r = v;
      *g = p;
      *b = q;
      break;
  }
}

uint32_t Decorator::blend(uint32_t from, uint32_t to, double ratio) {
  double h1,s1,v1, h2,s2,v2;
  rgb2hsv((from >> 16) / 255.0, ((from >> 8) & 0xff) / 255.0,
          (from & 0xff) / 255.0, &h1, &s1, &v1);
  rgb2hsv((to >> 16) / 255.0, ((to >> 8) & 0xff) / 255.0,
          (to & 0xff) / 255.0, &h2, &s2, &v2);
  double h, s, v;
  h = h1 * ratio + h2 * (1 - ratio);
  s = s1 * ratio + s2 * (1 - ratio);
  v = v1 * ratio + v2 * (1 - ratio);
  double r, g, b;
  hsv2rgb(h, s, v, &r, &g, &b);
  return (static_cast<int>(r * 255) << 16) |
      (static_cast<int>(g * 255) << 8) |
      static_cast<int>(b * 255);
}

void Decorator::embedIcon(const char *embed, uint8_t *out, int stride,
                          uint32_t color, uint32_t background, uint32_t scale) {
  auto png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr,
                                        nullptr, nullptr);
  auto info_ptr = png_create_info_struct(png_ptr);
  if (setjmp(png_jmpbuf(png_ptr))) {
    std::cerr << "PNG Failure" << std::endl;
    return;
  }
  FILE *f = fopen(embed, "rb");
  if (!f) {
    std::cerr << "Failed to read " << embed << std::endl;
    return;
  }
  png_init_io(png_ptr, f);
  png_read_info(png_ptr, info_ptr);

  uint32_t width, height;
  int bit_depth, color_type;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
               nullptr, nullptr, nullptr);
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
   png_set_expand(png_ptr);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand(png_ptr);
  }
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_expand(png_ptr);
  }
  if (bit_depth == 16) {
    png_set_strip_16(png_ptr);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png_ptr);
  }
  png_bytep row_pointers[height];
  png_read_update_info(png_ptr, info_ptr);
  uint32_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  uint8_t *imagedata = new uint8_t[rowbytes * height];
  for (int i = 0; i < height; i++) {
    row_pointers[i] = imagedata + i * rowbytes;
  }
  png_read_image(png_ptr, row_pointers);
  png_read_end(png_ptr, nullptr);
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
  fclose(f);

  double xlate = (double)width / scale;
  printf("width: %d, height: %d, xlate: %f, scale: %d\n",
         width, height, xlate, scale);

  for (int y = 0; y < scale; y++) {
    int src = (int)(y * xlate) * rowbytes;
    int dest = y * stride;
    for (int x = 0; x < scale; x++) {
      int sx = (int)(x * xlate) * 4;
      if (imagedata[src + sx + 3] == 0xff) {
        uint32_t c = blend(color, background, imagedata[src + sx] / 255.0);
        out[dest++] = c >> 16;
        out[dest++] = (c >> 8) & 0xff;
        out[dest++] = c & 0xff;
        out[dest++] = 0xff;
      } else {
        dest += 4;
      }
    }
  }
  delete [] imagedata;
}
