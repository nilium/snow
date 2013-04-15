#include "config.hh"

#include "dispatch.hh"
#include <Foundation/NSRunLoop.h>

#include "client/cl_main.hh"
#include "sys_main.hh"

int main(int argc, char const *argv[])
{
  using namespace snow;

  @autoreleasepool {
  // Queue up the actual main routine
  dispatch_async_s(dispatch_get_main_queue(), [argc, argv] {
    sys_init(argc, argv);
    client_t::get_client(client_t::DEFAULT_CLIENT_NUM).initialize(argc, argv);
  });
  [[NSRunLoop mainRunLoop] run];
  } // @autoreleasepool
  return 0;
}
