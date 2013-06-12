/*
  sv_main.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "sv_main.hh"
#include <snow-common.hh>
#include "../net/netevent.hh"
#include "../renderer/sgl.hh"
#include "../timing.hh"


namespace snow {

namespace {


server_t g_default_server;
const int SERVER_TIMEOUT = 2; // 1 second


} // namespace


server_t &server_t::get_server(size_t server_num)
{
  if (server_num != DEFAULT_SERVER_NUM)  {
    s_throw(std::out_of_range, "Invalid server number");
  }
  return g_default_server;
}



void server_t::initialize(int argc, const char **argv)
{
  ENetAddress host_addr;
  host_addr.host = ENET_HOST_ANY;
  host_addr.port = DEFAULT_SERVER_PORT;

  host_ = enet_host_create(&host_addr, num_clients_, 2, 0, 0);

  if (host_ == NULL) {
    s_throw(std::runtime_error, "Unable to create server host");
  }

  async_thread(&server_t::run_frameloop, this);
}



void server_t::run_frameloop()
{
  frameloop();
  shutdown();
}



void server_t::kill(bool block)
{
  running_ = false;
  while (!shutdown_) ;
}



void server_t::frameloop()
{
  #define NET_TIMEOUT 1

  running_ = true;

  base_time_ = glfwGetTime();
  sim_time_ = 0;
  int num_peers = 0;

  while (running_) {

    ENetEvent event;
    while (enet_host_service(host_, &event, NET_TIMEOUT) > 0) {
      s_log_note("Event received");
      switch (event.type) {
      case ENET_EVENT_TYPE_CONNECT: {
        s_log_note("Client connected");
        ++num_peers;

        netevent_t msg;
        msg.set_sender(0);
        msg.set_message(1);
        msg.set_time(sim_time_);
        msg.send(event.peer, 1, ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
      } break;

      case ENET_EVENT_TYPE_RECEIVE:
        enet_packet_destroy(event.packet);
      break;

      case ENET_EVENT_TYPE_DISCONNECT:
        s_log_note("Client disconnected");
        --num_peers;
      break;

      default:
      break;
      }
    }

    const double cur_time = glfwGetTime() - base_time_;
    while (sim_time_ < cur_time) {
      sim_time_ += FRAME_SEQ_TIME;
    }
  }
}



void server_t::shutdown()
{
  if (host_) {
    enet_host_flush(host_);
    enet_host_destroy(host_);
  }
  shutdown_ = true;
}


} // namespace snow
