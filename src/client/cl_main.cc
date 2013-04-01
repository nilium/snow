#include <iostream>
#include <memory>
#include <stdexcept>

#include "cl_main.hh"
#include "../event_queue.hh"
#include "../system.hh"
#include "../renderer/gl_error.hh"

#include <snow-common.hh>
#include <snow/math/math3d.hh>

// Font test
#include "../renderer/font.hh"
#include "../renderer/texture.hh"
#include "../ext/stb_image.h"
#include "../data/database.hh"


namespace snow {


#define GL_QUEUE_NAME    SNOW_ORG".gl_queue"
#define FRAME_QUEUE_NAME SNOW_ORG".frame_queue"


namespace {


client_t         g_client;
std::once_flag   g_init_flag;
dispatch_queue_t g_gl_queue = NULL;
dispatch_queue_t g_main_queue = NULL;



void cl_global_init();
void client_error_callback(int error, const char *msg);
void client_cleanup();



void cl_global_init()
{
  std::call_once(g_init_flag, [] {
    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwSetErrorCallback(client_error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#define USE_GLFW_HDPI_EXTENSION
#ifdef USE_GLFW_HDPI_EXTENSION
    glfwWindowHint(GLFW_HIDPI_IF_AVAILABLE, GL_TRUE);
#endif

    g_main_queue = dispatch_get_main_queue();
    g_gl_queue = dispatch_queue_create(GL_QUEUE_NAME, DISPATCH_QUEUE_SERIAL);

    s_log_note("---------------- STATIC INIT FINISHED ----------------");

    if (enet_initialize() != 0) {
      throw std::runtime_error("Error initializing enet - failing");
    }
  });
}



void client_error_callback(int error, const char *msg)
{
  s_log_error("GLFW Error [%d] %s", error, msg);
}



void client_cleanup()
{
  if (g_gl_queue)
    dispatch_release(g_gl_queue);

  glfwTerminate();
}


} // namespace <anon>



dispatch_queue_t cl_main_queue()
{
  cl_global_init();
  return g_main_queue;
}



dispatch_queue_t cl_gl_queue()
{
  cl_global_init();
  return g_gl_queue;
}



client_t &client_t::get_client(int client_num)
{
  if (client_num != DEFAULT_CLIENT_NUM)
    throw std::out_of_range("Invalid client number provided to client_t::get_client");

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



void client_t::dispose()
{
  if (is_connected())
    disconnect();

  if (frame_queue_) {
    dispatch_release(frame_queue_);
    frame_queue_ = nullptr;
  }

  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
}



// must be run on main queue
void client_t::terminate()
{
  dispose();
  client_cleanup();
  sys_quit();
}



void client_t::quit()
{
  running_.store(false);
}



// must be run on main queue
void client_t::initialize(int argc, const char *argv[])
{
  cl_global_init();

  s_log_note("Initializing window");
  window_ = glfwCreateWindow(800, 600, "Snow", NULL, NULL);
  if (!window_) {
    s_log_note("Window failed to initialize");
    throw std::runtime_error("Failed to create GLFW window");
  } else {
    s_log_note("Window initialized");
  }

  event_queue_.set_window_callbacks(window_, ALL_EVENT_KINDS);
  glfwSetInputMode(window_, GLFW_CURSOR_MODE, GLFW_CURSOR_HIDDEN);

  s_log_note("------------------- INIT FINISHED --------------------");
  s_log_note("Launching frameloop thread");

  // Launch frameloop thread
  async_thread(&client_t::run_frameloop, this);
}



bool client_t::connect(ENetAddress address)
{
  return false;
}



bool client_t::is_connected() const
{
  return false;
}



void client_t::disconnect()
{
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
