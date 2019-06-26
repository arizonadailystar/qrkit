#include <argp.h>
#include <iostream>
#include <cstdint>
#include "qrencoder.h"

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

  std::cout << arguments.message << std::endl;
  for (int i = 0; i < msg.length; i++) {
    printf("%02x ", msg.data[i]);
    if ((i & 15) == 15) {
      printf("\n");
    }
  }
  if (((msg.length - 1) & 15) != 15) {
    printf("\n");
  }
}
