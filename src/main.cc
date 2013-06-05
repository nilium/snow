#include "config.hh"
#include "client/cl_main.hh"
#include "sys_main.hh"


static void nullify_log_callback()
{
  s_set_log_callback(nullptr, nullptr);
}


int main(int argc, char const *argv[])
{
  using namespace snow;

  std::atexit(nullify_log_callback);
  sys_init(argc, argv);
  client_t::get_client(client_t::DEFAULT_CLIENT_NUM).initialize(argc, argv);

  return 0;
}
