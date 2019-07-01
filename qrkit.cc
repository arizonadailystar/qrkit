#include <argp.h>
#include <iostream>
#include <cstdint>
#include <png.h>
#include "qrencoder.h"
#include "qrgrid.h"
#include "colors.h"

const char *argp_program_version = "qrkit 0.5";
const char *argp_program_bug_address = "skasun@tucson.com";
static char doc[] = "Generate stylish QR codes";
static char args_doc[] = "message";
static struct argp_option options[] = {
  {"size", 'x', "SIZE", 0, "Size in pixels of final png (default 256)"},
  {"out", 'o', "FILENAME", 0, "Output filename (default qr.png)"},
  {"ecl", 'e', "L,M,Q,H", 0, "Minimum error correction level (default L)"},
  { 0 }
};

struct arguments {
  const char *outfile;
  std::string message;
  uint32_t size;
  ECL ecl;
};

static inline uint32_t parseNum(const char *s) {
  uint32_t res = 0;
  while (isspace(*s)) {
    s++;
  }
  while (isdigit(*s)) {
    res *= 10;
    res += *s - '0';
    s++;
  }
  return res;
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = static_cast<struct arguments *>(state->input);
  switch (key) {
    case 'x':
      if (arg) {
        arguments->size = parseNum(arg);
      }
      break;
    case 'o':
      arguments->outfile = arg;
      break;
    case 'e':
      switch (tolower(arg[0])) {
        case 'l':
          arguments->ecl = ECL::L;
          break;
        case 'm':
          arguments->ecl = ECL::M;
          break;
        case 'q':
          arguments->ecl = ECL::Q;
          break;
        case 'h':
          arguments->ecl = ECL::H;
          break;
        default:
          argp_usage(state);
          break;
      }
      break;
    case ARGP_KEY_ARG:
      if (!arguments->message.empty()) {
        arguments->message += " ";
      }
      arguments->message += arg;
      break;
    case ARGP_KEY_END:
      if (state->arg_num < 1) {
        argp_usage(state);
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char **argv) {
  struct arguments arguments;
  arguments.outfile = "qr.png";
  arguments.size = 256;
  arguments.ecl = ECL::L;
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  QREncoder encoder;
  Message msg = encoder.encode(arguments.message, arguments.ecl);

  QRGrid grid;
  Bitmap bitmap = grid.generate(msg);


  FILE *f = fopen("qr.png", "wb");

  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                                NULL, NULL);
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (setjmp(png_jmpbuf(png_ptr))) {
    printf("fail\n");
    return 0;
  }
  png_init_io(png_ptr, f);

  png_bytep row_pointers[bitmap.size];
  for (int row = 0; row < bitmap.size; row++) {
    row_pointers[row] = new uint8_t[bitmap.size];
    for (int x = 0; x < bitmap.size; x++) {
      switch (bitmap.data[row * bitmap.size + x]) {
        case Color::Empty:
        case Color::CodeOff:
        case Color::BG:
          row_pointers[row][x] = 0xff;
          break;
        default:
          row_pointers[row][x] = 0;
          break;
      }
    }
  }


  png_set_IHDR(png_ptr, info_ptr, bitmap.size, bitmap.size,
               8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);

  png_write_image(png_ptr, row_pointers);

  png_write_end(png_ptr, NULL);

  fclose(f);
}
