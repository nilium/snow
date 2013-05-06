#include "config.hh"
#include "client/cl_main.hh"
#include "sys_main.hh"


int main(int argc, char const *argv[])
{
  using namespace snow;

  sys_init(argc, argv);
  client_t::get_client(client_t::DEFAULT_CLIENT_NUM).initialize(argc, argv);

  return 0;
}
