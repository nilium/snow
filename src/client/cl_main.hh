#ifndef __SNOW_CL_MAIN_HH__
#define __SNOW_CL_MAIN_HH__

#include <snow/config.hh>
#include <snow/types/object_pool.hh>
#include "../net/netevent.hh"
#include "../event_queue.hh"
#include "../renderer/gl_state.hh"
#include <enet/enet.h>
#include <atomic>
#include <list>


struct GLFWwindow;


namespace snow {


struct system_t;


struct S_EXPORT client_t
{
  static const int DEFAULT_CLIENT_NUM = 0;

  static client_t &get_client(int client_num = DEFAULT_CLIENT_NUM);

  client_t();
  ~client_t();

  // Launches the client's frameloop thread. Should not be called more than once
  // per process (and only one client should exist per process).
  void initialize(int argc, const char *argv[]);
  // Kills the client frame loop and in turn the client itself. This will end
  // the process.
  void quit();

  // Attempts to connect to a server at the given address
  bool connect(ENetAddress address);
  // Whether connected to the server
  bool is_connected() const;
  // Disconnects from the server (if connected)
  void disconnect();

  gl_state_t &gl_state();
  const gl_state_t &gl_state() const;

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
  virtual void do_frame(double step, double timeslice);
  virtual void dispose();
  void pump_netevents(double timeslice);

private:
  using netevent_pool_t = object_pool_t<netevent_t, unsigned, false>;
  using system_pair_t = std::pair<int, system_t *>;

  std::atomic<bool>         running_ { false };
  double                    sim_time_ = 0;
  double                    base_time_ = 0;

  ENetHost *                host_ = NULL;
  ENetPeer *                peer_ = NULL;

  GLFWwindow *              window_ = NULL;
  dispatch_queue_t          frame_queue_ = NULL;
  dispatch_group_t          input_group_ = NULL;
  event_queue_t             event_queue_;
  gl_state_t                state_;
  std::list<system_pair_t>  systems_ { };
  netevent_pool_t           netevent_pool_;
};


S_EXPORT dispatch_queue_t cl_main_queue();


} // namespace snow

#endif /* end __SNOW_CL_MAIN_HH__ include guard */
