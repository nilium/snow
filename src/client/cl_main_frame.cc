#include "cl_main.hh"
#include "../game/system.hh"
#include "../game/gameobject.hh"
#include "../game/console_pane.hh"
#include "../game/systems/player.hh"
#include "../game/components/player_mover.hh"
#include "../renderer/gl_error.hh"
#include "../renderer/font.hh"
#include "../renderer/draw_2d.hh"
#include "../renderer/material.hh"
#include "../renderer/texture.hh"
#include "../renderer/buffer.hh"
#include "../renderer/program.hh"
#include "../renderer/shader.hh"
#include "../renderer/vertex_array.hh"
#include "../data/database.hh"
#include "../timing.hh"
#include <thread>
#include <physfs.h>
#include "../ext/fltk.h"


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



enum : int {
  EVENT_SENDER_UNKNOWN,
  EVENT_SENDER_NET,
};



std::chrono::milliseconds cl_frameloop_sleep_duration();
void cl_poll_events(void *ctx);



std::chrono::milliseconds cl_frameloop_sleep_duration()
{
  return std::chrono::milliseconds(16);
}



/*==============================================================================
  cl_poll_events

    Function to call glfwPollEvents on the main thread.
==============================================================================*/
void cl_poll_events(void *ctx)
{
  (void)ctx;
#define USE_FLTK_EVENT_POLLING 1
#if USE_FLTK_EVENT_POLLING
  while (Fl::wait(0) > 0)
    ;
#else
  glfwPollEvents();
#endif
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
#ifndef NDEBUG
  #define LONG_INPUT_TIME (0.004) /* 4 milliseconds */
  double input_time = glfwGetTime();
#endif

  event_queue_.set_frame_time(sim_time_);

#if USE_SERVER
  // Read network events
  if (is_connected()) {
    pump_netevents(timeslice);
  }
#endif

  cl_poll_events(nullptr);

#ifndef NDEBUG
  input_time = glfwGetTime() - input_time;
  if (input_time > LONG_INPUT_TIME) {
    s_log_warning("Input queue is taking a long time: %f", input_time);
  }
#endif

  // And run through events
  event_t event;
  auto sys_end = systems_.cend();
  auto sys_begin = systems_.cbegin();
  while (event_queue_.poll_event(event)) {
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
  for (const auto &spair : systems_) {
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

  player_t psystem_t;
  cvars_.clear();

  cvars_.register_ccmd(&cmd_quit_);
  cl_willQuit = cvars_.get_cvar( "cl_willQuit", 0, CVAR_READ_ONLY | CVAR_DELAYED | CVAR_INVISIBLE );
  wnd_focused = cvars_.get_cvar( "wnd_focused", 1, CVAR_READ_ONLY | CVAR_DELAYED | CVAR_INVISIBLE );
  wnd_mouseMode = cvars_.get_cvar("wnd_mouseMode", true, CVAR_DELAYED | CVAR_INVISIBLE);
  r_drawFrame = cvars_.get_cvar( "r_drawFrame", 1, CVAR_READ_ONLY | CVAR_DELAYED );

  add_system(&psystem_t);

  console.set_cvar_set(&cvars_);

  auto window = window_;
  glfwShowWindow(window);

  glfwMakeContextCurrent(window);
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

  rdraw_2d_t drawer;
  rbuffer_t indices(GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 2048);
  rbuffer_t vertices(GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 4096);
  rvertex_array_t vao;

  rfont_t *font = res_->load_font("console");
  assert(font);
  rmaterial_t *mat = res_->load_material("ui/console_font");
  rmaterial_t *pmat = res_->load_material("actors/player");
  rmaterial_t *bgmat = res_->load_material("ui/console_back");
  font->set_font_page(0, mat);
  console.set_font(font);
  console.set_background(bgmat);
  console.set_drawer(&drawer);

  add_system(&console, 65536);

  glClearColor(0.5, 0.5, 0.5, 1.0);
  glEnable(GL_BLEND);

  game_object_t *player = new game_object_t();
  player->add_component<player_mover_t>();

  for (auto &syspair : systems_) {
    s_log_note("%d %x", syspair.first, syspair.second);
  }

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
      glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_HIDDEN);
      glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
      break;
    case 1:
      glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_NORMAL);
      glfwSetInputMode(window, GLFW_STICKY_KEYS, 0);
      break;
    default: break;
    }
#endif

    if (frame != last_frame && r_drawFrame->geti()) {
      last_frame = frame;
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      assert_gl("Clearing buffers");

      for (auto &spair : systems_) {
        if (spair.second->active()) {
          spair.second->draw(sim_time_);
        }
      }

      drawer.draw_rect({400, 300}, {400, 300}, {1.0, 1.0, 0, 1.0}, pmat);
      drawer.buffer_vertices(vertices, 0);
      drawer.buffer_indices(indices, 0);

      if (!vao.generated()) {
        vao = drawer.build_vertex_array(ATTRIB_POSITION, ATTRIB_TEXCOORD0, ATTRIB_COLOR, vertices, 0, indices);
      }

      drawer.draw_with_vertex_array(vao, 0);

      glfwSwapBuffers(window);

      drawer.clear();
    }

    if (cl_willQuit->geti()) {
      running_ = false;
    } else if (!wnd_focused->geti()) {
      std::this_thread::sleep_for(cl_frameloop_sleep_duration());
    }
  } // while (running)

  s_set_log_callback(nullptr, nullptr);

  res_->release_all();

  delete player;

  glfwMakeContextCurrent(NULL);
} // frameloop


} // namespace snow
