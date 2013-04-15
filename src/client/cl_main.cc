#include <iostream>
#include <memory>
#include <stdexcept>

#include "cl_main.hh"
#include "../event_queue.hh"
#include "../sys_main.hh"
#include "../renderer/gl_error.hh"

#include <snow-common.hh>

#include "../server/sv_main.hh"

// Font test
#include "../renderer/font.hh"
#include "../renderer/texture.hh"
#include "../ext/stb_image.h"
#include "../data/database.hh"


namespace snow {


#ifndef USE_LOCAL_SERVER
#define USE_LOCAL_SERVER  (0)
#endif
#define UP_BANDWIDTH      (14400 / 8)
#define DOWN_BANDWIDTH    (57600 / 8)
#define GL_QUEUE_NAME     "net.spifftastic.snow.gl_queue"
#define FRAME_QUEUE_NAME  "net.spifftastic.snow.frame_queue"


namespace {


client_t         g_client;
std::once_flag   g_init_flag;
dispatch_queue_t g_main_queue = NULL;



void cl_global_init();
void client_cleanup();



void cl_global_init()
{
  std::call_once(g_init_flag, [] {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#define USE_GLFW_HDPI_EXTENSION
#ifdef USE_GLFW_HDPI_EXTENSION
    glfwWindowHint(GLFW_HIDPI_IF_AVAILABLE, GL_TRUE);
#endif

    g_main_queue = dispatch_get_main_queue();

    s_log_note("---------------- STATIC INIT FINISHED ----------------");

    if (enet_initialize() != 0) {
      s_throw(std::runtime_error, "Error initializing enet - failing");
    }
  });
}



void client_cleanup()
{
}


} // namespace <anon>



dispatch_queue_t cl_main_queue()
{
  cl_global_init();
  return g_main_queue;
}



client_t &client_t::get_client(int client_num)
{
  if (client_num != DEFAULT_CLIENT_NUM)
    s_throw(std::out_of_range, "Invalid client number provided to client_t::get_client");

  return g_client;
}



client_t::client_t() :
  running_(false),
  frame_queue_(dispatch_queue_create(FRAME_QUEUE_NAME, DISPATCH_QUEUE_CONCURRENT))
{
}



client_t::~client_t()
{
  dispose();
}



// must be run on main queue
void client_t::initialize(int argc, const char *argv[])
{
  cl_global_init();

  s_log_note("Initializing window");
  window_ = glfwCreateWindow(800, 600, "Snow", NULL, NULL);
  if (!window_) {
    s_log_note("Window failed to initialize");
    s_throw(std::runtime_error, "Failed to create GLFW window");
  } else {
    s_log_note("Window initialized");
  }

  event_queue_.set_window_callbacks(window_, ALL_EVENT_KINDS);
  glfwSetInputMode(window_, GLFW_CURSOR_MODE, GLFW_CURSOR_HIDDEN);

  s_log_note("------------------- INIT FINISHED --------------------");

  input_group_ = dispatch_group_create();

#if USE_LOCAL_SERVER
  // Create client host
  s_log_note("Creating local client");
  host_ = enet_host_create(NULL, 1, 2, DOWN_BANDWIDTH, UP_BANDWIDTH);
  if (host_ == NULL) {
    s_throw(std::runtime_error, "Unable to create client host");
  }

  s_log_note("Starting local server");
  server_t::get_server(server_t::DEFAULT_SERVER_NUM).initialize(argc, argv);

  s_log_note("Attempting to connect to server");
  ENetAddress server_addr;
  enet_address_set_host(&server_addr, "127.0.0.1");
  server_addr.port = server_t::DEFAULT_SERVER_PORT;
  if (!connect(server_addr)) {
    s_throw(std::runtime_error, "Unable to connect to local server");
  }
#endif

  // Launch frameloop thread
  s_log_note("Launching frameloop thread");
  async_thread(&client_t::run_frameloop, this);
}



void client_t::quit()
{
  running_.store(false);
}



bool client_t::connect(ENetAddress address)
{
  peer_ = enet_host_connect(host_, &address, 2, 0);

  if (peer_ == NULL) {
    s_log_error("Unable to allocate peer to connect to server");
    return false;
  }

  ENetEvent event;
  double timeout = glfwGetTime() + 5.0;
  int error = 0;
  while (glfwGetTime() < timeout) {
    while ((error = enet_host_service(host_, &event, 0)) > 0) {
      if (event.type == ENET_EVENT_TYPE_CONNECT && event.peer == peer_) {
        s_log_note("Established connection");
        return true;
      }
    }
    if (error < 0) {
      break;
    }
  }

  s_log_error("Unable to connect to host");
  enet_peer_reset(peer_);
  peer_ = NULL;

  return false;
}



bool client_t::is_connected() const
{
  return peer_ != NULL;
}



void client_t::disconnect()
{
  if (host_ != NULL) {
    enet_host_flush(host_);
    enet_peer_disconnect(peer_, 0);
    enet_host_destroy(host_);
    host_ = NULL;
  }
}



gl_state_t &client_t::gl_state()
{
  return state_;
}



const gl_state_t &client_t::gl_state() const
{
  return state_;
}



// must be run on main queue
void client_t::terminate()
{
  dispose();
  client_cleanup();
  sys_quit();
}



void client_t::dispose()
{
  if (is_connected()) {
    disconnect();
  }

  if (frame_queue_) {
    dispatch_release(frame_queue_);
    frame_queue_ = nullptr;
  }

  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }

#if USE_LOCAL_SERVER
  server_t::get_server(server_t::DEFAULT_SERVER_NUM).kill();
#endif
}



void client_t::add_system(system_t *system, int priority)
{
  auto iter = systems_.cbegin();
  auto end = systems_.cend();
  while (iter != end && iter->first > priority) {
    ++iter;
  }
  systems_.emplace(iter, priority, system);
}



void client_t::remove_system(system_t *system, int priority)
{
  auto iter = systems_.cbegin();
  auto end = systems_.cend();
  while (iter != end) {
    if (iter->first == priority && iter->second == system) {
      iter = systems_.erase(iter);
    } else if (priority > iter->first) {
      break;
    } else {
      ++iter;
    }
  }
}



void client_t::remove_system(system_t *system)
{
  auto iter = systems_.cbegin();
  auto end = systems_.cend();
  while (iter != end) {
    if (iter->second == system) {
      iter = systems_.erase(iter);
    } else {
      ++iter;
    }
  }
}



void client_t::remove_all_systems()
{
  systems_.clear();
}


} // namespace snow
