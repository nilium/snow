#ifdef TARGET_OS_MAC
#include <dispatch/dispatch.h>
#include <Foundation/Foundation.h>
#endif

#include <unistd.h>

#include "client/cl_main.hh"

int main(int argc, char const *argv[])
{
  using namespace snow;
  // Bootstrap
#ifdef TARGET_OS_MAC
    @autoreleasepool {
#endif
  // Queue up the actual main routine
  dispatch_async(dispatch_get_main_queue(), [&] {
    client_t::get_client(client_t::DEFAULT_CLIENT_NUM).initialize(argc, argv);
  });
#ifdef TARGET_OS_MAC
  [[NSRunLoop mainRunLoop] run];
  } // @autoreleasepool
#else
  dispatch_main();
#endif
  return 0;
}
