/*
  event_queue.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW_EVENT_QUEUE_HH__
#define __SNOW_EVENT_QUEUE_HH__

#include "config.hh"
#include "event.hh"
#include "ext/zmqxx.hh"

#include <memory>
#include <mutex>


namespace snow {


#define EVENT_ENDPOINT ("inproc://events")


// Note: Not thread safe. Interact with the event queue object only from a
// single thread. To read events, connect to the event endpoint and receive
// events.
// Creating multiple event queues is fine, provided they each use a different
// socket. Using a PUSH-PULL socket, you can reasonably easily create a many-to-
// -one combined event queue using multiple event queues to emit events.
struct S_EXPORT event_queue_t
{
  bool          emit_event(const event_t &event);
  void          set_frame_time(double time);

  void          set_window_callbacks(GLFWwindow *window, int events_mask = ALL_EVENT_KINDS);

  // Sets the write socket. If the socket is set to NULL, events will be dropped
  // from the queue. The socket must already be bound to a given endpoint.
  void          set_socket(zmq::socket_t *socket);

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

  zmq::socket_t *   write_socket_ = nullptr;
  double            last_time_ = 0;
  double            frame_time_ = 0;
};


} // namespace snow

#endif /* end __SNOW_EVENT_QUEUE_HH__ include guard */
