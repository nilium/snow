#include "config.hh"
#include "client/cl_main.hh"
#include "sys_main.hh"
#include <cfloat>


static void nullify_log_callback()
{
  snow::s_set_log_callback(nullptr, nullptr);
}


int main(int argc, char const *argv[])
{
  using namespace snow;

#ifdef S_USE_CONTROLFP
  _controlfp(_PC_24, _MCW_PC);
  _controlfp(_RC_NEAR, _MCW_RC);
#endif

  std::atexit(nullify_log_callback);
  sys_init(argc, argv);
  client_t::get_client(client_t::DEFAULT_CLIENT_NUM).initialize(argc, argv);

  return 0;
}
