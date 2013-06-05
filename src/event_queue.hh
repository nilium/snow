#ifndef __SNOW_EVENT_QUEUE_HH__
#define __SNOW_EVENT_QUEUE_HH__

#include "config.hh"
#include "event.hh"

#include <list>
#include <memory>
#include <mutex>
#include <tbb/mutex.h>


namespace snow {


struct S_EXPORT event_queue_t
{
  using event_list_t = std::list<event_t>;

  bool          peek_event(event_t &out) const;
  bool          poll_event(event_t &out);
  bool          poll_event_before(event_t &out, double time);
  void          emit_event(const event_t &event);
  void          set_frame_time(double time);
  void          clear();

  // Get a copy of the event queue
  event_list_t  event_queue() const;

  void          set_window_callbacks(GLFWwindow *window, int events_mask = ALL_EVENT_KINDS);

private:

  static void   ecb_key_event(GLFWwindow *window, int key, int action, int mods);
  static void   ecb_mouse_event(GLFWwindow *window, int button, int action, int mods);
  static void   ecb_char_event(GLFWwindow *window, unsigned int character);
  static void   ecb_mouse_pos_event(GLFWwindow *window, double x, double y);
  static void   ecb_mouse_scroll_event(GLFWwindow *window, double x, double y);
  static void   ecb_mouse_enter_event(GLFWwindow *window, int entered);
  static void   ecb_window_close_event(GLFWwindow *window);
  static void   ecb_window_move_event(GLFWwindow *window, int x, int y);
  static void   ecb_window_size_event(GLFWwindow *window, int width, int height);
  static void   ecb_window_focus_event(GLFWwindow *window, int focused);
  static void   ecb_window_iconify_event(GLFWwindow *window, int iconified);

#if USE_LOCKED_EVENT_QUEUE
  mutable tbb::mutex lock_;
#endif
  event_list_t      events_;
  double            last_time_ = 0;
  double            frame_time_ = 0;
  event_list_t::const_iterator last_event_;
};


} // namespace snow

#endif /* end __SNOW_EVENT_QUEUE_HH__ include guard */
