#pragma once

#include <snow/config.hh>

namespace murmur3 {

using string = snow::string;

const uint32_t DEFAULT_HASH_SEED_32 = 0x9E2030F1U;

S_EXPORT uint32_t hash32(const string &str, uint32_t seed = DEFAULT_HASH_SEED_32);

S_EXPORT uint32_t hash32(const char *str, const size_t length, uint32_t seed = DEFAULT_HASH_SEED_32);

S_EXPORT uint64_t hash64(const string &str, uint32_t seed = DEFAULT_HASH_SEED_32);

S_EXPORT uint64_t hash64(const char *str, const size_t length, uint32_t seed = DEFAULT_HASH_SEED_32);

}
