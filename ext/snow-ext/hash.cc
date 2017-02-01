#include "hash.hh"
#include "murmur3.h"

namespace murmur3 {


uint32_t hash32(const string &str, uint32_t seed)
{
  return hash32(str.c_str(), str.size(), seed);
}

uint64_t hash64(const string &str, uint32_t seed)
{
  return hash64(str.c_str(), str.size(), seed);
}

uint64_t hash64(const char *str, const size_t length, uint32_t seed)
{
  union {
    struct {
      uint64_t lower;
      uint64_t upper;
    } block;
    char raw[16];
  } buf;
  MurmurHash3_x86_128(str, static_cast<int>(length), seed, buf.raw);
  return buf.block.upper;
}

uint32_t hash32(const char *str, const size_t length, uint32_t seed)
{
  uint32_t result = 0;
  MurmurHash3_x86_32(str, static_cast<int>(length), seed, static_cast<void *>(&result));
  return result;
}


}

