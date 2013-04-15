#include "cl_main.hh"
#include "../game/system.hh"
#include "../game/console_pane.hh"
#include "../renderer/gl_error.hh"
#include "../renderer/font.hh"
#include "../renderer/draw_2d.hh"
#include "../renderer/material_basic.hh"
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


cvar_t cl_willQuit ( "cl_willQuit",  0, CVAR_READ_ONLY | CVAR_DELAYED );
cvar_t wnd_focused ( "wnd_focused",  1, CVAR_READ_ONLY | CVAR_DELAYED );
cvar_t r_drawFrame ( "r_drawFrame",  1, CVAR_READ_ONLY | CVAR_DELAYED );


enum : int {
  EVENT_SENDER_UNKNOWN,
  EVENT_SENDER_NET,
};


struct esc_system_t : system_t
{
  virtual bool event(const event_t &event)
  {
    if (event.kind == KEY_EVENT && event.key.button == GLFW_KEY_ESC) {
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
      cl_willQuit.seti(1);
    }
  }

private:
  bool will_quit_ = false;
};



const std::chrono::milliseconds &cl_frameloop_sleep_duration();
void cl_poll_events(void *ctx);



const std::chrono::milliseconds &cl_frameloop_sleep_duration()
{
  static std::chrono::milliseconds sleep_duration(10);
  return sleep_duration;
}



rshader_t load_shader(gl_state_t &gl, GLenum kind, const string &path)
{
  auto file = PHYSFS_openRead(path.c_str());
  if (!file) {
    s_throw(std::runtime_error, "Cannot open file %s", path.c_str());
  }

  auto size = PHYSFS_fileLength(file);
  std::vector<char> filebuf(size);
  PHYSFS_readBytes(file, filebuf.data(), size);
  PHYSFS_close(file);

  rshader_t shader(gl, kind);
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
void client_t::pump_netevents(double timeslice)
{
  #define NET_TIMEOUT 1

  ENetEvent event;
  int error = 0;
  while ((error = enet_host_service(host_, &event, NET_TIMEOUT)) > 0) {
    if (event.type == ENET_EVENT_TYPE_RECEIVE) {
      if (event.packet) {
        const auto index = netevent_pool_.reserve();
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
  #define LONG_INPUT_TIME (0.004) /* 4 milliseconds */

  double input_time = glfwGetTime();
  // Read network events
  if (is_connected()) {
    dispatch_group_async_s(input_group_, frame_queue_, [this, timeslice] {
      pump_netevents(timeslice);
    });

    // Read user input
    dispatch_group_async_f(input_group_, cl_main_queue(), NULL, cl_poll_events);
    // Allow the group to take up to 4 milliseconds to poll for events before
    // it just goes ahead
    dispatch_group_wait(input_group_, DISPATCH_TIME_FOREVER);
  } else {
    dispatch_sync_f(cl_main_queue(), NULL, cl_poll_events);
  }

  input_time = glfwGetTime() - input_time;
  if (input_time > LONG_INPUT_TIME) {
    s_log_warning("Input queue is taking a long time: %f", input_time);
  }

  // And run through events
  event_t event;
  auto sys_end = systems_.cend();
  auto sys_begin = systems_.cbegin();
  while (event_queue_.poll_event_before(event, base_time_ + timeslice)) {

    if (event.kind == WINDOW_FOCUS_EVENT) {
      wnd_focused.seti(event.focused);
      continue;
    }

    auto sys_iter = sys_begin;
    event.time -= base_time_;
    while (sys_iter != sys_end) {
      system_t *sys = sys_iter->second;
      if (sys->active() && !sys->event(event)) {
        break;
      }
      ++sys_iter;
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
  esc_system_t debug_sys;
  add_system(&debug_sys);

  frameloop();

  remove_system(&debug_sys);

  // Go back to the main thread and kill the program cleanly.
  dispatch_async_s(cl_main_queue(), [this] { terminate(); });
}



/*==============================================================================
  frameloop

    Body of the frameloop. Handles incoming events, signals system updates, and
    frame rendering.
==============================================================================*/
void client_t::frameloop()
{
  cvars_.clear();
  cvars_.register_cvar(&cl_willQuit);
  cvars_.register_cvar(&r_drawFrame);

  console_pane_t &console = default_console();
  console.set_cvar_set(&cvars_);

  auto window = window_;
  gl_state_t &gl = state_;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(0);    // Don't needlessly limit rendering speed
  gl.acquire();

  // Set this to true before getting the base time since it might loop a couple
  // times setting it.
  running_ = true;
  sim_time_ = 0;
  // Don't call glfwSetTime because that might throw other clients out of sync
  // (even though really the chance of there being other clients is zero)
  base_time_ = glfwGetTime();
  unsigned frame, last_frame;
  frame = 1; last_frame = 0;

  rdraw_2d_t drawer(gl);
  rbuffer_t indices(gl, GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 2048);
  rbuffer_t vertices(gl, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW, 4096);
  rvertex_array_t vao = drawer.build_vertex_array(0, 1, 2, vertices, 0, true);
  rprogram_t prog(gl);
  build_program(prog,
    load_shader(gl, GL_VERTEX_SHADER, "basic.vsh"),
    load_shader(gl, GL_FRAGMENT_SHADER, "basic.fsh"));

  database_t fontdb = database_t::read_physfs("console_font.db");
  rfont_t font(fontdb, "PragmataPro");
  fontdb.close();
  rtexture_t tex = load_texture_2d(gl, "console_font.png", true, TEX_COMP_RGBA);
  rmaterial_basic_t mat(gl);
  mat.set_program(&prog, 0, 1, 2);
  mat.set_texture(&tex);
  font.set_font_page(0, &mat);
  console.set_font(&font);
  console.set_drawer(&drawer);
  console.set_background(&mat);

  add_system(&console);

  glClearColor(0.5, 0.5, 0.5, 1.0);
  glEnable(GL_BLEND);
  gl.set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  while (running_.load()) {

    // Do any frames that would have passed since the last rendering point
    const double cur_time = glfwGetTime() - base_time_;
    while (sim_time_ < cur_time) {
      sim_time_ += FRAME_SEQ_TIME;
      ++frame;
      read_events(sim_time_);
      do_frame(FRAME_SEQ_TIME, sim_time_);
      cvars_.update_cvars();
    }


    if (frame != last_frame && r_drawFrame.geti()) {
      last_frame = frame;
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      assert_gl("Clearing buffers");

      for (auto &spair : systems_) {
        if (spair.second->active()) {
          spair.second->draw(sim_time_);
        }
      }

      font.draw_text(drawer, { 200, 200 }, "foobar");
      drawer.buffer_vertices(vertices, 0);
      drawer.buffer_indices(indices, 0);
      drawer.draw_with_vertex_array(vao, indices, 0);

      glfwSwapBuffers(window);

      drawer.clear();
    }

    if (cl_willQuit.geti()) {
      running_ = false;
    } else if (!wnd_focused.geti()) {
      std::this_thread::sleep_for(cl_frameloop_sleep_duration());
    }
  } // while (running)

  glfwMakeContextCurrent(NULL);
} // frameloop


} // namespace snow
