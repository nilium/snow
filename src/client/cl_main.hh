#ifndef __SNOW_CL_MAIN_HH__
#define __SNOW_CL_MAIN_HH__

#include "../snow-config.hh"
#include "../event_queue.hh"
#include <enet/enet.h>
#include <atomic>
#include "../renderer/gl_state.hh"


struct GLFWwindow;


namespace snow {


struct S_EXPORT client_t
{
  static const int DEFAULT_CLIENT_NUM = 0;

  static client_t &get_client(int client_num = DEFAULT_CLIENT_NUM);

  client_t();
  virtual ~client_t();

  // Launches the client's frameloop thread. Should not be called more than once
  // per process (and only one client should exist per process).
  virtual void initialize(int argc, const char *argv[]);
  // Kills the client frame loop and in turn the client itself. This will end
  // the process.
  virtual void quit();

  // Attempts to connect to a server at the given address
  virtual bool connect(ENetAddress address);
  // Whether connected to the server
  virtual bool is_connected() const;
  // Disconnects from the server (if connected)
  virtual void disconnect();

  inline gl_state_t &gl_state() { return glstate_; }

protected:
  virtual void terminate();
  virtual void run_frameloop();
  virtual void frameloop();
  virtual void dispose();

private:
  ENetHost *host_;
  dispatch_queue_t frame_queue_;
  GLFWwindow *window_;
  event_queue_t event_queue_;
  double last_frame_;
  gl_state_t glstate_;

  std::atomic<bool> running_;
};


S_EXPORT dispatch_queue_t cl_get_gl_queue();


} // namespace snow

#endif /* end __SNOW_CL_MAIN_HH__ include guard */
