/** @copyright 2019 Arizona Daily Star.  Developed by Sean Kasun. */

#include <sstream>
#include <strings.h>
#include "json.h"

enum Token {
  TokenNULL,
  TokenTRUE,
  TokenFALSE,
  TokenString,
  TokenNumber,
  TokenObject,
  TokenArray,
  TokenObjectClose,
  TokenArrayClose,
  TokenKeySeparator,
  TokenValueSeparator,
};

class JSONHelper {
 public:
  explicit JSONHelper(const std::string &data) : data(data) {
    pos = 0;
    len = data.length();
  }

  Token nextToken() {
    while (pos < len && std::isspace(data[pos])) {
      pos++;
    }
    checkBounds();
    auto c = data[pos++];
    if (std::isalpha(c)) {  // keyword like NULL, TRUE, or FALSE
      int start = pos - 1;
      while (pos < len && std::isalpha(data[pos])) {
        pos++;
      }
      auto ref = data.substr(start, pos - start);
      if (strcasecmp("null", ref.c_str()) == 0) {
        return TokenNULL;
      }
      if (strcasecmp("true", ref.c_str()) == 0) {
        return TokenTRUE;
      }
      if (strcasecmp("false", ref.c_str()) == 0) {
        return TokenFALSE;
      }
      throw JSONParseException("Unquoted string", location());
    }
    if (std::isdigit(c) || c == '-') {  // double or hex
      pos--;
      return TokenNumber;
    }
    switch (c) {
      case '"':
        return TokenString;
      case '{':
        return TokenObject;
      case '}':
        return TokenObjectClose;
      case '[':
        return TokenArray;
      case ']':
        return TokenArrayClose;
      case ':':
        return TokenKeySeparator;
      case ',':
        return TokenValueSeparator;
      default:
        std::string s = "Unexpected character: ";
        s += c;
        throw JSONParseException(s, location());
    }
  }

  std::string readString() {
    std::string r;
    while (pos < len && data[pos] != '"') {
      if (data[pos] == '\\') {
        pos++;
        checkBounds();
        switch (data[pos++]) {
          case '"':
            r += '"';
            break;
          case '\\':
            r += '\\';
            break;
          case '/':
            r += '/';
            break;
          case 'b':
            r += '\b';
            break;
          case 'f':
            r += '\f';
            break;
          case 'n':
            r += '\n';
            break;
          case 'r':
            r += '\r';
            break;
          case 't':
            r += '\t';
            break;
          case 'u':
            {
              uint64_t cp = readHex();
              uint32_t low = 0;
              if (cp >= 0xd800 && cp < 0xdc00) {  // surrogates
                if (data[pos] == '\\' && data[pos + 1] == 'u') {
                  pos += 2;
                  low = readHex();
                }
                if (low < 0xdc00 || low >= 0xe000) {
                  throw JSONParseException("Incomplete Surrogate pair",
                                           location());
                }
                cp = (cp << 10) + low - 0x35fdc00;
              }
              // to utf8
              if (cp < 0x80) {
                r += (char)cp;
              } else if (cp < 0x800) {
                r += (char)(0xc0 | (cp >> 6));
                r += (char)(0x80 | (cp & 0x3f));
              } else if (cp < 0x10000) {
                r += (char)(0xe0 | (cp >> 12));
                r += (char)(0x80 | ((cp >> 6) & 0x3f));
                r += (char)(0x80 | (cp & 0x3f));
              } else {
                r += (char)(0xf0 | (cp >> 18));
                r += (char)(0x80 | ((cp >> 12) & 0x3f));
                r += (char)(0x80 | ((cp >> 6) & 0x3f));
                r += (char)(0x80 | (cp & 0x3f));
              }
            }
            break;
          default:
            throw JSONParseException("Unknown escape character", location());
        }
      } else {
        r += data[pos++];
      }
    }
    pos++;
    return r;
  }

  uint32_t readHex() {
    uint32_t num = 0;
    for (int i = 0; i < 4; i++) {
      checkBounds();
      num <<= 4;
      auto c = data[pos++];
      if (c >= '0' && c <= '9') {
        num |= c - '0';
      } else if (c >= 'a' && c <= 'f') {
        num |= c - 'a' + 10;
      } else if (c >= 'A' && c <= 'F') {
        num |= c - 'A' + 10;
      } else {
        throw JSONParseException("Invalid hex code", location());
      }
    }
    return num;
  }

  double readDouble() {
    double sign = 1.0;
    if (data[pos] == '-') {
      sign = -1.0;
      pos++;
    } else if (data[pos] == '+') {
      pos++;
    }
    double value = 0.0;
    while (pos < len && std::isdigit(data[pos])) {
      value *= 10.0;
      value += data[pos++] - '0';
    }
    checkBounds();
    if (data[pos] == '.') {
      double pow10 = 10.0;
      pos++;
      while (pos < len && std::isdigit(data[pos])) {
        value += (data[pos++] - '0') / pow10;
        pow10 *= 10.0;
      }
    }
    checkBounds();
    double scale = 1.0;
    bool frac = false;
    if (data[pos] == 'e' || data[pos] == 'E') {
      pos++;
      checkBounds();
      if (data[pos] == '-') {
        frac = true;
        pos++;
      } else if (data[pos] == '+') {
        pos++;
      }
      unsigned int expon = 0;
      while (pos < len && std::isdigit(data[pos])) {
        expon *= 10.0;
        expon += data[pos++] - '0';
      }
      if (expon > 308) {
        expon = 308;
      }
      while (expon >= 50) {
        scale *= 1E50;
        expon -= 50;
      }
      while (expon >= 8) {
        scale *= 1E8;
        expon -= 8;
      }
      while (expon > 0) {
        scale *= 10.0;
        expon--;
      }
    }
    return sign * (frac ? (value / scale) : (value * scale));
  }

  std::string location() {
    int line = 1;
    int col = 0;
    int cpos = pos;
    bool doneCol = false;
    while (cpos >= 0) {
      if (data[cpos] == '\n') {
        doneCol = true;
        line++;
      }
      if(!doneCol) {
        col++;
      }
      cpos--;
    }
    std::stringstream ss;
    ss << "Line: " << line << " Offset: " << col;
    return ss.str();
  }

 private:
  int pos, len;
  std::string data;

  void checkBounds() {
    if (pos == len) {
      throw JSONParseException("Unexpected EOF", location());
    }
  }
};

const std::shared_ptr<JSONData> JSON::parse(const std::string &data) {
  if (data.length() == 0) {
    return nullptr;
  }

  JSONHelper reader(data);
  Token type = reader.nextToken();
  switch (type) {
    case TokenObject:
      return std::make_shared<JSONObject>(reader);
    case TokenArray:
      return std::make_shared<JSONArray>(reader);
    default:
      throw JSONParseException("Doesn't start with object or array",
                               reader.location());
  }
  return nullptr;
}

static std::shared_ptr<JSONData> Null = std::make_shared<JSONData>();

JSONData::JSONData() {}
JSONData::~JSONData() {}
bool JSONData::has(const std::string &) const {
  return false;
}
const std::shared_ptr<JSONData> JSONData::at(const std::string &) const {
  return Null;
}
const std::shared_ptr<JSONData> JSONData::at(int) const {
  return Null;
}
int JSONData::length() const {
  return 0;
}
const std::string JSONData::asString() const {
  return "";
}
double JSONData::asNumber() const {
  return 0.0;
}
bool JSONData::asBool() const {
  return false;
}
static ChildMap emptymap;
const ChildMap::const_iterator JSONData::begin() const {
  return emptymap.begin();
}
const ChildMap::const_iterator JSONData::end() const {
  return emptymap.end();
}

JSONBool::JSONBool(bool val) {
  data = val;
}
bool JSONBool::asBool() const {
  return data;
}

JSONString::JSONString(const std::string &val) {
  data = val;
}
const std::string JSONString::asString() const {
  return data;
}
double JSONString::asNumber() const {
  JSONHelper helper(data + " ");  // add a space delimiter for sanity
  return helper.readDouble();
}

JSONNumber::JSONNumber(double val) {
  data = val;
}

double JSONNumber::asNumber() const {
  return data;
}

JSONObject::JSONObject(JSONHelper &reader) {
  Token type;
  while ((type = reader.nextToken()) != TokenObjectClose) {
    if (type != TokenString) {
      throw JSONParseException("Expected quoted string", reader.location());
    }
    std::string key = reader.readString();
    if (key.length() == 0) {
      throw JSONParseException("Empty object key", reader.location());
    }
    if (reader.nextToken() != TokenKeySeparator) {
      throw JSONParseException("Expected ':'", reader.location());
    }
    std::shared_ptr<JSONData> value;
    switch (reader.nextToken()) {
      case TokenNULL:
        value = nullptr;
        break;
      case TokenTRUE:
        value = std::make_shared<JSONBool>(true);
        break;
      case TokenFALSE:
        value = std::make_shared<JSONBool>(false);
        break;
      case TokenString:
        value = std::make_shared<JSONString>(reader.readString());
        break;
      case TokenNumber:
        value = std::make_shared<JSONNumber>(reader.readDouble());
        break;
      case TokenObject:
        value = std::make_shared<JSONObject>(reader);
        break;
      case TokenArray:
        value = std::make_shared<JSONArray>(reader);
        break;
      default:
        throw JSONParseException("Expected value", reader.location());
    }
    children[key] = value;
    type = reader.nextToken();  // comma or end
    if (type == TokenObjectClose) {
      break;
    }
    if (type != TokenValueSeparator) {
      throw JSONParseException("Expected ',' or '}'", reader.location());
    }
  }
}

const ChildMap::const_iterator JSONObject::begin() const {
  return children.begin();
}

const ChildMap::const_iterator JSONObject::end() const {
  return children.end();
}

bool JSONObject::has(const std::string &key) const {
  return children.find(key) != children.end();
}

const std::shared_ptr<JSONData> JSONObject::at(const std::string &key) const {
  auto i = children.find(key);
  if (i != children.end()) {
    return i->second;
  }
  return Null;
}

JSONArray::JSONArray(JSONHelper &reader) {
  Token type;
  while ((type = reader.nextToken()) != TokenArrayClose) {
    std::shared_ptr<JSONData> value;
    switch (type) {
      case TokenNULL:
        value = nullptr;
        break;
      case TokenTRUE:
        value = std::make_shared<JSONBool>(true);
        break;
      case TokenFALSE:
        value = std::make_shared<JSONBool>(false);
        break;
      case TokenString:
        value = std::make_shared<JSONString>(reader.readString());
        break;
      case TokenNumber:
        value = std::make_shared<JSONNumber>(reader.readDouble());
        break;
      case TokenObject:
        value = std::make_shared<JSONObject>(reader);
        break;
      case TokenArray:
        value = std::make_shared<JSONArray>(reader);
        break;
      default:
        throw JSONParseException("Expected Value", reader.location());
    }
    data.push_back(value);
    type = reader.nextToken();  // comma or end
    if (type == TokenArrayClose) {
      break;
    }
    if (type != TokenValueSeparator) {
      throw JSONParseException("Expected ',' or ']'", reader.location());
    }
  }
}

int JSONArray::length() const {
  return data.size();
}

const std::shared_ptr<JSONData> JSONArray::at(int index) const {
  if (index >=0 && index < data.size()) {
    return data[index];
  }
  return Null;
}
