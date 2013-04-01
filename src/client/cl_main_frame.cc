#include "cl_main.hh"
#include "../game/system.hh"
#include "../renderer/gl_error.hh"
#include "../renderer/font.hh"
#include "../data/database.hh"

namespace snow {


namespace {


struct esc_system_t : system_t
{
  virtual bool event(const event_t &event, client_t &client)
  {
    if (event.kind == KEY_EVENT && event.key.button == GLFW_KEY_ESC) {
      will_quit_ = true;
      return true;
    } else {
      return false;
    }
  }

  virtual void frame(double step, client_t &client)
  {
    if (will_quit_) {
      client.quit();
    }
  }

private:
  bool will_quit_ = false;
};


void cl_poll_events(void *ctx);



void cl_poll_events(void *ctx)
{
  (void)ctx;
  glfwPollEvents();
}


} // namespace <anon>


void client_t::pump_netevents()
{

}



void client_t::read_events(double timeslice)
{
  // Read network events
  pump_netevents();

  // Read user input
  dispatch_sync_f(cl_main_queue(), NULL, cl_poll_events);

  event_t event;
  auto sys_end = systems_.cend();
  auto sys_begin = systems_.cbegin();
  while (event_queue_.poll_event_before(event, base_time_ + timeslice)) {
    auto sys_iter = sys_begin;
    event.time -= base_time_;
    while (sys_iter != sys_end) {
      system_t *sys = sys_iter->second;
      if (sys->active() && !sys->event(event, *this)) {
        break;
      }
      ++sys_iter;
    }
  }
}



void client_t::do_frame(double step)
{
  for (const auto &spair : systems_) {
    if (spair.second->active()) {
      spair.second->frame(step, *this);
    }
  }
}



void client_t::run_frameloop()
{
  esc_system_t debug_sys;
  add_system(&debug_sys);

  database_t db("fonts.db", true, SQLITE_OPEN_READONLY);
  rfont_t font(db, "HelveticaNeue");
  db.close();

  try {
    frameloop();
  } catch (std::exception &ex) {
    s_log_error("Uncaught exception in frameloop: %s", ex.what());
  }

  remove_system(&debug_sys);

  // Go back to the main thread and kill the program cleanly.
  dispatch_async(cl_main_queue(), [this] { terminate(); });

}



// Run as single thread
void client_t::frameloop()
{
  auto window = window_;
  gl_state_t &gl = state_;

  dispatch_sync(cl_gl_queue(), [window, &gl] {
    glfwMakeContextCurrent(window);
    gl.acquire();

    glfwSwapInterval(0);
  });

  running_ = true;

  sim_time_ = 0;
  // Don't call glfwSetTime because that might throw other clients out of sync
  base_time_ = glfwGetTime();


  while (running_.load()) {

    const double cur_time = glfwGetTime() - base_time_;
    while (sim_time_ < cur_time) {
      sim_time_ += FRAME_SEQ_TIME;
      read_events(sim_time_);
      do_frame(FRAME_SEQ_TIME);
    }

    dispatch_async(cl_gl_queue(), [window] {
      glfwMakeContextCurrent(window);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      assert_gl("Clearing buffers");

      // renderer_.do_frame();
    });

    dispatch_sync(cl_gl_queue(), [window] {
      glfwMakeContextCurrent(window);
      glfwSwapBuffers(window);
    });
  } // while (running)
} // frameloop


} // namespace snow
