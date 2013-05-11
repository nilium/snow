#include "cl_main.hh"
#include "../game/system.hh"
#include "../game/console_pane.hh"
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



struct esc_system_t : system_t
{
  esc_system_t(cvar_t *willQuit) :
    cl_willQuit_(willQuit)
  {
    // nop
  }

  virtual bool event(const event_t &event)
  {
    if (event.kind == KEY_EVENT && event.key.button == GLFW_KEY_PAUSE) {
      will_quit_ = true;
      return true;
    } else if (event.kind == NET_EVENT) {
      s_log_note("Net event: %d %d %f",
        event.net->sender(), event.net->message(), event.net->time());
      return true;
    } else {
      return false;
    }
  }

  virtual void frame(double step, double timeslice)
  {
    if (will_quit_) {
      s_log_note("Should quit");
      cl_willQuit_->seti(1);
    }
  }

private:
  cvar_t *cl_willQuit_ = nullptr;
  bool will_quit_ = false;
};



const std::chrono::milliseconds &cl_frameloop_sleep_duration();
void cl_poll_events(void *ctx);



const std::chrono::milliseconds &cl_frameloop_sleep_duration()
{
  static std::chrono::milliseconds sleep_duration(10);
  return sleep_duration;
}



rshader_t load_shader(GLenum kind, const string &path)
{
  auto file = PHYSFS_openRead(path.c_str());
  if (!file) {
    s_throw(std::runtime_error, "Cannot open file %s", path.c_str());
  }

  auto size = PHYSFS_fileLength(file);
  std::vector<char> filebuf(size);
  PHYSFS_readBytes(file, filebuf.data(), size);
  PHYSFS_close(file);

  rshader_t shader(kind);
  shader.load_source(string(filebuf.data(), filebuf.size()));

  if (!shader.compile()) {
    s_throw(std::runtime_error, "Couldn't compile shader: %s", shader.error_string().c_str());
  }

  return shader;
}



bool build_program(rprogram_t &program, const rshader_t &vertex, const rshader_t &frag)
{
  program.bind_uniform(0, "modelview");
  program.bind_uniform(1, "projection");
  program.bind_uniform(2, "diffuse");
  program.bind_attrib(0, "position");
  program.bind_attrib(1, "texcoord");
  program.bind_attrib(2, "color");
  program.bind_frag_out(0, "frag_color");

  program.attach_shader(vertex);
  program.attach_shader(frag);

  if (!program.link()) {
    s_throw(std::runtime_error, "Couldn't link program: %s", program.error_string().c_str());
    return false;
  }

  return true;
}



/*==============================================================================
  cl_poll_events

    Function to call glfwPollEvents on the main thread.
==============================================================================*/
void cl_poll_events(void *ctx)
{
  (void)ctx;
  glfwPollEvents();
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

  glfwPollEvents();

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
      case WINDOW_FOCUS_EVENT: {
        wnd_focused->seti(event.focused);
        continue;
      }

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
  console_pane_t &console = default_console();
  s_set_log_callback(cl_log_callback, &console);

  cvars_.clear();

  cvars_.register_ccmd(&cmd_quit_);
  cl_willQuit = cvars_.get_cvar( "cl_willQuit", 0, CVAR_READ_ONLY | CVAR_DELAYED | CVAR_INVISIBLE );
  wnd_focused = cvars_.get_cvar( "wnd_focused", 1, CVAR_READ_ONLY | CVAR_DELAYED | CVAR_INVISIBLE );
  wnd_mouseMode = cvars_.get_cvar("wnd_mouseMode", false, CVAR_DELAYED | CVAR_INVISIBLE);
  r_drawFrame = cvars_.get_cvar( "r_drawFrame", 1, CVAR_READ_ONLY | CVAR_DELAYED );

  esc_system_t debug_sys(cl_willQuit);
  add_system(&debug_sys);

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
  rvertex_array_t vao = drawer.build_vertex_array(ATTRIB_POSITION, ATTRIB_TEXCOORD0, ATTRIB_COLOR, vertices, 0);

  rfont_t *font = res_->load_font("console");
  assert(font);
  rmaterial_t *mat = res_->load_material("ui/console_font");
  rmaterial_t *bgmat = res_->load_material("ui/console_back");
  font->set_font_page(0, mat);
  console.set_font(font);
  console.set_background(bgmat);
  console.set_drawer(&drawer);

  add_system(&console);

  glClearColor(0.5, 0.5, 0.5, 1.0);
  glEnable(GL_BLEND);

  while (running_.load()) {
    int mousemode = -1;

    // Do any frames that would have passed since the last rendering point
    const double cur_time = glfwGetTime() - base_time_;
    while (sim_time_ < cur_time) {
      sim_time_ += FRAME_SEQ_TIME;
      ++frame;
      read_events(sim_time_);
      do_frame(FRAME_SEQ_TIME, sim_time_);

      if (wnd_mouseMode->has_flags(CVAR_MODIFIED)) {
        wnd_mouseMode->update();
        mousemode = wnd_mouseMode->geti();
      }

      cvars_.update_cvars();
    }

    switch (mousemode) {
    case 0: glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_HIDDEN); break;
    case 1: glfwSetInputMode(window, GLFW_CURSOR_MODE, GLFW_CURSOR_NORMAL); break;
    default: break;
    }

    if (frame != last_frame && r_drawFrame->geti()) {
      last_frame = frame;
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      assert_gl("Clearing buffers");

      for (auto &spair : systems_) {
        if (spair.second->active()) {
          spair.second->draw(sim_time_);
        }
      }

      drawer.buffer_vertices(vertices, 0);
      drawer.buffer_indices(indices, 0);
      drawer.draw_with_vertex_array(vao, indices, 0);

      glfwSwapBuffers(window);

      drawer.clear();
    }

    if (cl_willQuit->geti()) {
      running_ = false;
    } else if (!wnd_focused->geti()) {
      std::this_thread::sleep_for(cl_frameloop_sleep_duration());
    }
  } // while (running)

  res_->release_all();
  s_set_log_callback(nullptr, nullptr);

  glfwMakeContextCurrent(NULL);
} // frameloop


} // namespace snow
