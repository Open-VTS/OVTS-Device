/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Utility library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/
#ifndef utility_h
#define utility_h
#include "mbed.h"
#include "mbedtls/base64.h"
#include "string"
#include "vector"

class Utility
{
public:
  static void remove_string(std::string &str, const char *to_delete);
  // read std::string between quotes on another std::string
  static std::string read_between_quotes(std::string input);
  // append char* into beginning of char array
  static void removeSubstr(char *s, const char *toremove);
  static void str_prepend(unsigned char *s, const char *t);
  static std::vector<std::string> split(const std::string &s, const char delim);

private:
  std::vector<std::string> &split_helper(const std::string &s, const char delim, std::vector<std::string> &elems);
};

class Base64
{
public:
  Base64(void);
  static int encode(unsigned char *buffer, int buffer_size, const unsigned char *message, int message_length);
  static int decode(unsigned char *buffer, int buffer_size, const unsigned char *message);
  //determine base64 encode size
  static int encode_size(int input_size);
};

#endif
