/** @copyright 2019 Sean Kasun */

#include <argp.h>
#include <iostream>
#include <cstdint>
#include <fstream>
#include "qrencoder.h"
#include "qrgrid.h"
#include "colors.h"
#include "config.h"
#include "decorator.h"

const char *argp_program_version = "qrkit 0.5";
const char *argp_program_bug_address = "skasun@tucson.com";
static char doc[] = "Generate stylish QR codes";
static char args_doc[] = "message";
static struct argp_option options[] = {
  {"config", 'c', "FILENAME", 0, "Name of config file"},
  {"out", 'o', "FILENAME", 0, "Output filename (default qr.png)"},
  { 0 }
};

struct arguments {
  const char *outfile;
  const char *config;
  std::string message;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = static_cast<struct arguments *>(state->input);
  switch (key) {
    case 'c':
      arguments->config = arg;
      break;
    case 'o':
      arguments->outfile = arg;
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
  arguments.config = "config.json";
  argp_parse(&argp, argc, argv, 0, 0, &arguments);


  std::ifstream f(arguments.config, std::ios::in | std::ios::binary |
                         std::ios::ate);
  std::string jsonstring;

  if (!f.is_open()) {
    std::cerr << "Failed to open " << arguments.config << std::endl;
  } else {
    int len = f.tellg();
    char *data = new char[len + 1];
    f.seekg(0, std::ios::beg);
    f.read(data, len);
    data[len] = 0;
    f.close();
    jsonstring = data;
    delete [] data;
  }

  std::shared_ptr<JSONData> json;
  try {
    json = JSON::parse(jsonstring);
  } catch (JSONParseException e) {
    std::cerr << e.reason << std::endl;
    return -1;
  }

  Config config(json);

  QREncoder encoder;
  Message msg = encoder.encode(arguments.message, config.minECL);

  QRGrid grid;
  Bitmap bitmap = grid.generate(msg);

  Decorator::decorate(bitmap, config, arguments.outfile);
}
