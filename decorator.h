/** @copyright 2019 Sean Kasun */

#include "qrgrid.h"
#include "config.h"
#include "colors.h"

class Decorator {
 public:
  static void decorate(const Bitmap &bitmap, const Config &config,
                       const char *filename);

 private:
  static uint32_t getColor(Color color, const Config &config);
  static void drawDot(uint8_t *out, int stride, uint32_t color, uint32_t scale,
                      uint8_t mask);
};
