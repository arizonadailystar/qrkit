/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

#include <argp.h>
#include <iostream>
#include <cstdint>
#include <fstream>
#include "qrencoder.h"
#include "qrgrid.h"
#include "colors.h"
#include "config.h"
#include "decorator.h"

const char *argp_program_version = "qrkit 0.6";
const char *argp_program_bug_address = "mobile@tucson.com";
static char doc[] = "Generate stylish QR codes";
static char args_doc[] = "message";
static struct argp_option options[] = {
  {"config", 'c', "FILENAME", 0, "Name of config file"},
  {"embed", 'e', "FILENAME", 0, "Image to embed in middle"},
  {"out", 'o', "FILENAME", 0, "Output filename (default qr.png)"},
  {"ppi_x", 1000, "INTEGER", 0, "Horizontal pixels per inch (ignored by default)"},
  {"ppi_y", 1001, "INTEGER", 0, "Vertical pixels per inch (ignored by default)"},
  { 0 }
};

struct arguments {
  const char *outfile;
  const char *config;
  const char *embed;
  std::string message;
  unsigned int ppi_x, ppi_y;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = static_cast<struct arguments *>(state->input);
  switch (key) {
    case 'c':
      arguments->config = arg;
      break;
    case 'e':
      arguments->embed = arg;
      break;
    case 'o':
      arguments->outfile = arg;
      break;
    case 1000:
      arguments->ppi_x = atoi(arg);
      break;
    case 1001:
      arguments->ppi_y = atoi(arg);
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
  arguments.embed = nullptr;
  arguments.ppi_x = 0;
  arguments.ppi_y = 0;
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

  Decorator::decorate(bitmap, config, arguments.embed, arguments.outfile, arguments.ppi_x, arguments.ppi_y);
}
