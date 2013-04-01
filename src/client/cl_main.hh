#ifndef __SNOW_CL_MAIN_HH__
#define __SNOW_CL_MAIN_HH__

#include "../snow-config.hh"
#include "../event_queue.hh"
#include "../renderer/gl_state.hh"
#include <enet/enet.h>
#include <atomic>
#include <list>


struct GLFWwindow;


namespace snow {


constexpr double FRAME_SECOND = 1.0;
constexpr double FRAME_HERTZ = 60.0;
constexpr double FRAME_SEQ_TIME = FRAME_SECOND / FRAME_HERTZ;


struct system_t;


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

  inline gl_state_t &gl_state() { return state_; }

  /* Adds a system to the list of systems to update/send events to. Does not
  check to see if the system is already in the list. */
  void add_system(system_t *system, int priority = 0);
  /* Removes a system if and only if its priority matches */
  void remove_system(system_t *system, int priority);
  /* Removes a system regardless of what its priority is */
  void remove_system(system_t *system);
  void remove_all_systems();

protected:
  void terminate();
  void run_frameloop();
  void frameloop();
  virtual void read_events(double timeslice);
  virtual void do_frame(double step);
  virtual void dispose();
  void pump_netevents();

private:
  using system_pair_t = std::pair<int, system_t *>;

  std::atomic<bool> running_;
  double sim_time_;
  double base_time_;

  ENetHost *host_ = NULL;

  GLFWwindow *window_ = NULL;
  dispatch_queue_t frame_queue_;
  event_queue_t event_queue_;
  gl_state_t state_;
  std::list<system_pair_t> systems_;
};


S_EXPORT dispatch_queue_t cl_main_queue();
S_EXPORT dispatch_queue_t cl_gl_queue();


} // namespace snow

#endif /* end __SNOW_CL_MAIN_HH__ include guard */
