#ifndef __SNOW_EVENT_QUEUE_HH__
#define __SNOW_EVENT_QUEUE_HH__

#include <deque>
#include <iomanip>
#include <iostream>
#include <memory>

#include <dispatch/dispatch.h>
#include <gl/glfw3.h>

#include "snow-config.hh"
#include <snow/types/types_2d.hh>


namespace snow {


enum event_kind_t : int
{
  NULL_EVENT           = 0,
  KEY_EVENT            = 0x1 << 0,
  CHAR_EVENT           = 0x1 << 1,
  MOUSE_EVENT          = 0x1 << 2,
  MOUSE_MOVE_EVENT     = 0x1 << 3,
  MOUSE_SCROLL_EVENT   = 0x1 << 4,
  MOUSE_ENTER_EVENT    = 0x1 << 5,
  WINDOW_CLOSE_EVENT   = 0x1 << 6,
  WINDOW_FOCUS_EVENT   = 0x1 << 7,
  WINDOW_ICONIFY_EVENT = 0x1 << 8,
  WINDOW_SIZE_EVENT    = 0x1 << 9,
  WINDOW_MOVE_EVENT    = 0x1 << 10,
  OPAQUE_EVENT         = 0x1 << 11,
  ALL_KEY_EVENTS       = (KEY_EVENT | CHAR_EVENT),
  ALL_MOUSE_EVENTS     = (MOUSE_EVENT |
                          MOUSE_MOVE_EVENT |
                          MOUSE_SCROLL_EVENT |
                          MOUSE_ENTER_EVENT),
  ALL_WINDOW_EVENTS    = (WINDOW_CLOSE_EVENT |
                          WINDOW_FOCUS_EVENT |
                          WINDOW_ICONIFY_EVENT |
                          WINDOW_SIZE_EVENT |
                          WINDOW_MOVE_EVENT),
  ALL_EVENT_KINDS      = (~NULL_EVENT)
};



S_EXPORT const string &event_kind_string(event_kind_t kind);

const int EVENT_SENDER_UNKNOWN = 0;
const int EVENT_SENDER_WINDOW = -1;



struct S_EXPORT button_event_t {
  int button;
  int action;
};



struct S_EXPORT event_t
{
  int sender_id;
  union {
    void *sender;
    GLFWwindow *window;
  };
  event_kind_t kind;
  double time;
  union {
    button_event_t key;             // KEY_EVENT
    int            character;       // CHAR_EVENT
    button_event_t mouse;           // MOUSE_EVENT
    pointi_t       mouse_pos;       // MOUSE_MOVE_EVENT
    pointd_t       scroll;          // MOUSE_SCROLL_EVENT
    bool           entered;         // MOUSE_ENTER_EVENT
    bool           focused;         // WINDOW_FOCUS_EVENT
    bool           iconified;       // WINDOW_ICONIFY_EVENT
    dimensi_t      window_size;     // WINDOW_SIZE_EVENT
    pointi_t       window_pos;      // WINDOW_MOVE_EVENT
    void          *opaque;          // OPAQUE_EVENT
  };

  // If you know the sender_id is valid for this
  template <typename T, int ID>
  constexpr T *cast_sender() const;
};



template <typename T, int ID>
constexpr T *event_t::cast_sender() const
{
  return (sender_id == ID ? (T *)sender : nullptr);
}



inline
std::ostream &operator << (std::ostream &out, const button_event_t &in)
{
  return
  (out << "{ button: " << in.button
       << ", action: " << (in.action == GLFW_PRESS
                           ? "GLFW_PRESS }"
                           : (in.action == GLFW_RELEASE
                              ? "GLFW_RELEASE }"
                              : "GLFW_REPEAT }")));
}



inline
std::ostream &operator << (std::ostream &out, const event_t &in)
{
  out << "{ window: " << std::showbase << std::hex << (void*)in.window
      << ", kind: " << event_kind_string(in.kind)
      << std::resetiosflags(~0);
  switch (in.kind) {
  case KEY_EVENT:
    out << ", key: " << in.key;
    break;
  case MOUSE_EVENT:
    out << ", mouse: " << in.mouse;
    break;
  case CHAR_EVENT:
    out << ", character: " << in.character;
    break;
  case MOUSE_MOVE_EVENT:
    out << ", mouse_pos: " << in.mouse_pos;
    break;
  case MOUSE_SCROLL_EVENT:
    out << ", scroll: " << in.scroll;
    break;
  case MOUSE_ENTER_EVENT:
    out << ", entered: " << std::boolalpha << in.entered;
    break;
  case WINDOW_FOCUS_EVENT:
    out << ", focused: " << std::boolalpha << in.focused;
    break;
  case WINDOW_ICONIFY_EVENT:
    out << ", iconified: " << std::boolalpha << in.iconified;
    break;
  case WINDOW_SIZE_EVENT:
    out << ", window_size: " << in.window_size;
    break;
  case WINDOW_MOVE_EVENT:
    out << ", window_pos: " << in.window_pos;
    break;
  case OPAQUE_EVENT:
    out << ", opaque: " << std::showbase << std::hex << in.opaque;
    break;
  default: break;
  }
  return (out << " }");
}



struct S_EXPORT event_queue_t
{
  using event_list_t = std::deque<event_t>;


  event_queue_t();
  ~event_queue_t();

  bool          wait_event(event_t &out, dispatch_time_t timeout = DISPATCH_TIME_FOREVER);
  bool          peek_event(event_t &out) const;
  bool          poll_event(event_t &out);
  bool          poll_event_before(event_t &out, double time);
  void          emit_event(const event_t &event);

  // Get a copy of the event queue
  event_list_t  event_queue() const;

  void          set_window_callbacks(GLFWwindow *window, int events_mask = ALL_EVENT_KINDS);

private:
  dispatch_queue_t  queue_;
  event_list_t      events_;
};


} // namespace snow

#endif /* end __SNOW_EVENT_QUEUE_HH__ include guard */
