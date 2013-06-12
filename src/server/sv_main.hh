/*
  sv_main.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW_SV_MAIN_HH__
#define __SNOW_SV_MAIN_HH__

#include "../config.hh"
#include <enet/enet.h>
#include <atomic>

namespace snow {

struct server_t
{
  static const int DEFAULT_SERVER_PORT = 23208;
  static const size_t DEFAULT_SERVER_NUM = 0;

  static server_t &get_server(size_t server_num);

  void initialize(int argc, const char **argv);
  void run_frameloop();
  // By default, blocks until the server has been completely killed. If block
  // is false, it will kill the server and return without waiting for it to
  // finish.
  void kill(bool block = true);

private:
  void frameloop();
  void shutdown();

  std::atomic<bool> shutdown_ { false };
  std::atomic<bool> running_ { false };
  int num_clients_ = 16;
  ENetHost *host_ = NULL;
  double base_time_ = 0.0;
  double sim_time_ = 0.0;
};


} // namespace snow

#endif /* end __SNOW_SV_MAIN_HH__ include guard */
