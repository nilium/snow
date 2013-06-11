#include "cl_main.hh"
#include "../game/system.hh"
#include "../game/console_pane.hh"
#include "../renderer/gl_error.hh"
#include "../timing.hh"
#include <thread>


namespace snow {


namespace {



static void cl_log_callback(const char *msg, size_t len, void *ctx)
{
  console_pane_t *console = (console_pane_t *)ctx;
  if (!ctx) {
    return;
  }
  console->write_log(string(msg, len));
}



std::chrono::milliseconds cl_frameloop_sleep_duration()
{
  return std::chrono::milliseconds(16);
}


} // namespace <anon>



/*==============================================================================
  pump_netevents

    Reads events from the server or other connections and inserts them into the
    event queue.
==============================================================================*/
#if USE_SERVER
void client_t::pump_netevents(double timeslice)
{
  #define NET_TIMEOUT 1

  ENetEvent event;
  int error = 0;
  while ((error = enet_host_service(host_, &event, NET_TIMEOUT)) > 0) {
    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
      if (event.packet) {
        const auto index = netevent_pool_.allocate();
        netevent_t &netevent = netevent_pool_[index];
        netevent.read_from(event.packet);
        enet_packet_destroy(event.packet);
        event_t emitted = {
          EVENT_SENDER_NET,
          { .sender = this },
          NET_EVENT,
          timeslice
        };
        emitted.net = &netevent;
        event_queue_.emit_event(emitted);
        s_log_note("Emitted net event");
      }
    }
  }

  if (error < 0) {
    s_log_error("Error checking for ENet events: %d", error);
  }
}
#endif



/*==============================================================================
  read_events(timeslice)

    Reads events that have occurred prior to the given timeslice. Events that
    occur afterward must be ignored and left in the event queue.

    Any events valid to the timeslice must be propagated through all systems.
    Systems may receive events in an out-of-order manner due to netevents being
    emitted after polling for OS events.
==============================================================================*/
void client_t::read_events(double timeslice)
{
  event_queue_.set_frame_time(sim_time_);

  // And run through events
  event_t event;
  auto sys_end = logic_systems_.cend();
  auto sys_begin = logic_systems_.cbegin();
  while (read_socket_.recv(&event, sizeof(event), ZMQ_DONTWAIT) == sizeof(event)) {
    switch (event.kind) {
      case WINDOW_FOCUS_EVENT:
        wnd_focused->seti(event.focused);
        // fall-through

      default: {
        auto sys_iter = sys_begin;
        event.time -= base_time_;
        while (sys_iter != sys_end) {
          system_t *sys = sys_iter->second;
          if (sys->active() && !sys->event(event)) {
            break;
          }
          ++sys_iter;
        }
        break;
      }
    }
  }
}



/*==============================================================================
  do_frame(step, timeslice)

    Perform a single frame for as much time has passed as step. Barring changes
    to gameplay, step should be considered a constant some fraction of a second
    measured in seconds.
==============================================================================*/
void client_t::do_frame(double step, double timeslice)
{
  for (const auto &spair : logic_systems_) {
    if (spair.second->active()) {
      spair.second->frame(step, timeslice);
    }
  }
}



/*==============================================================================
  run_frameloop

    Kick off the frameloop. This just calls frameloop() and should not allocate
    any resources itself -- if resources must be allocated, it should be done
    so in or beneath frameloop() and freed before frameloop() ends. This is to
    prevent async hiccups when terminate() is called on the main thread.
==============================================================================*/
void client_t::run_frameloop()
{
  frameloop();
  terminate();
}



/*==============================================================================
  frameloop

    Body of the frameloop. Handles incoming events, signals system updates, and
    frame rendering.
==============================================================================*/
void client_t::frameloop()
{
  // FIXME: Almost all of this crap should be moved to game-specific code.
  console_pane_t &console = default_console();
  s_set_log_callback(cl_log_callback, &console);

  cvars_.clear();
  cvars_.register_ccmd(&cmd_quit_);

  cl_willQuit = cvars_.get_cvar( "cl_willQuit", 0, CVAR_READ_ONLY | CVAR_DELAYED | CVAR_INVISIBLE );
  wnd_focused = cvars_.get_cvar( "wnd_focused", 1, CVAR_READ_ONLY | CVAR_DELAYED | CVAR_INVISIBLE );
  wnd_mouseMode = cvars_.get_cvar("wnd_mouseMode", true, CVAR_DELAYED | CVAR_INVISIBLE);
  r_drawFrame = cvars_.get_cvar( "r_drawFrame", 1, CVAR_READ_ONLY | CVAR_DELAYED );
  r_clearFrame = cvars_.get_cvar("r_clearFrame", 1, CVAR_READ_ONLY | CVAR_DELAYED );

  console.set_cvar_set(&cvars_);

  glfwShowWindow(window_);
  glfwMakeContextCurrent(window_);
  glfwSwapInterval(0);    // Don't needlessly limit rendering speed

  // Set this to true before getting the base time since it might loop a couple
  // times setting it.
  running_ = true;
  sim_time_ = 0;
  // Don't call glfwSetTime because that might throw other clients out of sync
  // (even though really the chance of there being other clients is zero)
  base_time_ = glfwGetTime();
  unsigned frame, last_frame;
  frame = 1; last_frame = 0;

  // FIXME: move these somewhere else, probably?
  glClearColor(0, 0, 0, 1);
  glEnable(GL_BLEND);

  add_system(&console, 16777216, -16777216);

  while (running_.load()) {
#if HIDE_CURSOR_ON_CONSOLE_CLOSE
    int mousemode = -1;
#endif

    // Do any frames that would have passed since the last rendering point
    const double cur_time = glfwGetTime() - base_time_;
    while (sim_time_ < cur_time) {
      sim_time_ += FRAME_SEQ_TIME;
      ++frame;
      read_events(sim_time_);
      do_frame(FRAME_SEQ_TIME, sim_time_);

#if HIDE_CURSOR_ON_CONSOLE_CLOSE
      if (wnd_mouseMode->has_flags(CVAR_MODIFIED)) {
        wnd_mouseMode->update();
        mousemode = wnd_mouseMode->geti();
      }
#endif

      cvars_.update_cvars();
    }

#if HIDE_CURSOR_ON_CONSOLE_CLOSE
    switch (mousemode) {
    case 0:
      glfwSetInputMode(window_, GLFW_CURSOR_MODE, GLFW_CURSOR_HIDDEN);
      glfwSetInputMode(window_, GLFW_STICKY_KEYS, 1);
      break;
    case 1:
      glfwSetInputMode(window_, GLFW_CURSOR_MODE, GLFW_CURSOR_NORMAL);
      glfwSetInputMode(window_, GLFW_STICKY_KEYS, 0);
      break;
    default: break;
    }
#endif

    if (frame != last_frame && r_drawFrame->geti()) {
      last_frame = frame;

      if (r_clearFrame->geti()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        assert_gl("Clearing buffers");
      }

      for (auto &spair : draw_systems_) {
        if (spair.second->active()) {
          spair.second->draw(sim_time_);
        }
      }

      glfwSwapBuffers(window_);
    }

    if (cl_willQuit->geti()) {
      running_ = false;
    } else if (!wnd_focused->geti()) {
      std::this_thread::sleep_for(cl_frameloop_sleep_duration());
    }
  } // while (running)

  s_set_log_callback(nullptr, nullptr);

  res_->release_all();

  glfwMakeContextCurrent(NULL);
} // frameloop


} // namespace snow
