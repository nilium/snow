#include "config.hh"
#include "dispatch.hh"
#include "client/cl_main.hh"
#include "sys_main.hh"

int main(int argc, char const *argv[])
{
  using namespace snow;
  // Queue up the actual main routine
  dispatch_async_s(dispatch_get_main_queue(), [argc, argv] {
    sys_init(argc, argv);
    client_t::get_client(client_t::DEFAULT_CLIENT_NUM).initialize(argc, argv);
  });

  dispatch_main();

  return 0;
}
