#include <snow/data/hash.hh>
#include <cstdint>
#include <cstdio>
#include <cstring>

int main(int argc, char const *argv[])
{
  for (size_t index = 1; index < argc; ++index) {
    const size_t len = std::strlen(argv[index]);
    const uint32_t h32 = snow::hash32(argv[index], len);
    const uint64_t h64 = snow::hash64(argv[index], len);
    printf("%#010xU %#018llxUL <%s>\n", h32, h64, argv[index]);
  }
  return 0;
}