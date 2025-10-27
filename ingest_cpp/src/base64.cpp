#include "brain/base64.hpp"
#include <string>

static const char table[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const unsigned char* data, size_t len) {
  std::string out;
  out.reserve(((len+2)/3)*4);
  for (size_t i=0; i<len; i+=3) {
    unsigned int n = (data[i] << 16);
    if (i+1 < len) n |= (data[i+1] << 8);
    if (i+2 < len) n |= data[i+2];

    out.push_back(table[(n >> 18) & 63]);
    out.push_back(table[(n >> 12) & 63]);
    out.push_back((i+1<len) ? table[(n >> 6) & 63] : '=');
    out.push_back((i+2<len) ? table[n & 63] : '=');
  }
  return out;
}
