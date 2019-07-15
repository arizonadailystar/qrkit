/** @copyright 2019 Sean Kasun */

#pragma once

#include <memory>
#include <map>
#include <vector>

class JSONHelper;

typedef std::map<std::string, std::shared_ptr<class JSONData>> ChildMap;

class JSONData {
 public:
  JSONData();
  virtual ~JSONData();
  virtual bool has(const std::string &key) const;
  virtual const std::shared_ptr<JSONData> at(const std::string &key) const;
  virtual const std::shared_ptr<JSONData> at(int index) const;
  virtual int length() const;
  virtual const std::string asString() const;
  virtual double asNumber() const;
  virtual bool asBool() const;
  virtual const ChildMap::const_iterator begin() const;
  virtual const ChildMap::const_iterator end() const;
};

class JSONBool : public JSONData {
 public:
  explicit JSONBool(bool val);
  bool asBool() const;

 private:
  bool data;
};

class JSONString : public JSONData {
 public:
  explicit JSONString(const std::string &val);
  const std::string asString() const;
  double asNumber() const;

 private:
  std::string data;
};

class JSONNumber : public JSONData {
 public:
  explicit JSONNumber(double val);
  double asNumber() const;

 private:
  double data;
};

class JSONObject : public JSONData {
 public:
  explicit JSONObject(JSONHelper &);
  bool has(const std::string &key) const;
  const std::shared_ptr<JSONData> at(const std::string &key) const;
  const ChildMap::const_iterator begin() const;
  const ChildMap::const_iterator end() const;

 private:
  ChildMap children;
};

class JSONArray : public JSONData {
 public:
  explicit JSONArray(JSONHelper &);
  const std::shared_ptr<JSONData> at(int index) const;
  int length() const;

 private:
  std::vector<std::shared_ptr<JSONData>> data;
};

class JSONParseException {
 public:
  JSONParseException(std::string reason, std::string at) :
    reason(reason + " at " + at) {}
  std::string reason;
};

class JSON {
 public:
  static const std::shared_ptr<JSONData> parse(const std::string &data);
};
