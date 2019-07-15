/** @copyright 2019 Sean Kasun */

#include "qrgrid.h"
#include "config.h"
#include "colors.h"

class Decorator {
 public:
  static void decorate(const Bitmap &bitmap, const Config &config,
                       const char *embed, const char *filename);

 private:
  static uint32_t getColor(uint8_t color, const Config &config);
  static void embedIcon(const char *embed, uint8_t *out, int stride,
                        uint32_t color, uint32_t background, uint32_t scale);
  static void drawDot(uint8_t *out, int stride, uint32_t color,
                      uint32_t background, uint32_t scale, uint8_t mask);
  static void drawPattern(uint8_t *out, int stride, uint32_t color,
                          uint32_t background, uint32_t scale,
                          PatternStyle style, uint8_t corners);
  static void drawSquare(uint8_t *out, int stride, uint32_t color,
                         uint32_t background, uint32_t scale);
  static void drawRounded(uint8_t *out, int stride, uint32_t color,
                          uint32_t background, uint32_t scale, uint8_t corners);
  static void drawCircle(uint8_t *out, int stride, uint32_t color,
                          uint32_t background, uint32_t scale);
  static uint32_t blend(uint32_t from, uint32_t to, double ratio);
  static void rgb2hsv(double r, double g, double b,
                      double *h, double *s, double *v);
  static void hsv2rgb(double h, double s, double v,
                      double *r, double *g, double *b);
};
