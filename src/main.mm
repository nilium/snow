/*
  main.mm -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "config.hh"

#include "dispatch.hh"
#include <Foundation/NSRunLoop.h>

#include "client/cl_main.hh"
#include "sys_main.hh"
#include "ext/lexer.hh"
#include <iomanip>

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
