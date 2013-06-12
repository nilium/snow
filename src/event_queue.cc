/*
  event_queue.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "event_queue.hh"
#include <atomic>
#include <map>

namespace snow {


namespace {


/*******************************************************************************
*                                Event mappings                                *
*******************************************************************************/

using event_name_map_t = std::map<event_kind_t, string>;
const event_name_map_t g_event_names {
  std::make_pair(NULL_EVENT,           "NULL_EVENT"),
  std::make_pair(KEY_EVENT,            "KEY_EVENT"),
  std::make_pair(CHAR_EVENT,           "CHAR_EVENT"),
  std::make_pair(MOUSE_EVENT,          "MOUSE_EVENT"),
  std::make_pair(MOUSE_MOVE_EVENT,     "MOUSE_MOVE_EVENT"),
  std::make_pair(MOUSE_SCROLL_EVENT,   "MOUSE_SCROLL_EVENT"),
  std::make_pair(MOUSE_ENTER_EVENT,    "MOUSE_ENTER_EVENT"),
  std::make_pair(WINDOW_CLOSE_EVENT,   "WINDOW_CLOSE_EVENT"),
  std::make_pair(WINDOW_FOCUS_EVENT,   "WINDOW_FOCUS_EVENT"),
  std::make_pair(WINDOW_ICONIFY_EVENT, "WINDOW_ICONIFY_EVENT"),
  std::make_pair(WINDOW_SIZE_EVENT,    "WINDOW_SIZE_EVENT"),
  std::make_pair(WINDOW_MOVE_EVENT,    "WINDOW_MOVE_EVENT"),
  std::make_pair(OPAQUE_EVENT,         "OPAQUE_EVENT"),
};


} // namespace <anon>


/*******************************************************************************
*                           Event string conversion                            *
*******************************************************************************/

const string &event_kind_string(event_kind_t kind)
{
  event_name_map_t::const_iterator iter = g_event_names.find(kind);
  if (iter == g_event_names.cend()) {
    s_throw(std::out_of_range, "Invalid event kind");
  }
  return iter->second;
}



/*******************************************************************************
*                         event_queue_t implementation                         *
*******************************************************************************/



bool event_queue_t::emit_event(const event_t &event)
{
  /* If no peer to receive it, drop the event */
  if (write_socket_) {
    int result = write_socket_->send(&event, sizeof(event), ZMQ_DONTWAIT);
    assert(result == sizeof(event) || (result == -1 && errno == EAGAIN));
    return result == sizeof(event);
  }
  return false;
}



void event_queue_t::set_frame_time(double time)
{
  frame_time_ = time;
}



void event_queue_t::set_socket(zmq::socket_t *socket)
{
  write_socket_ = socket;
}



void event_queue_t::set_window_callbacks(GLFWwindow *window, int events_mask)
{
  #define flag_check(FLAGS, FLAG) (((FLAGS)&(FLAG))==(FLAG))

  glfwSetWindowUserPointer(window, (events_mask ? (void *)this : NULL));

  glfwSetKeyCallback(window,
    (flag_check(events_mask, KEY_EVENTS)
     ? ecb_key_event
     : NULL));

  glfwSetCharCallback(window,
    (flag_check(events_mask, CHAR_EVENTS)
     ? ecb_char_event
     : NULL));

  glfwSetMouseButtonCallback(window,
    (flag_check(events_mask, MOUSE_EVENTS)
     ? ecb_mouse_event
     : NULL));

  glfwSetCursorPosCallback(window,
    (flag_check(events_mask, MOUSE_MOVE_EVENTS)
     ? ecb_mouse_pos_event
     : NULL));

  glfwSetScrollCallback(window,
    (flag_check(events_mask, MOUSE_SCROLL_EVENTS)
     ? ecb_mouse_scroll_event
     : NULL));

  glfwSetCursorEnterCallback(window,
    (flag_check(events_mask, MOUSE_ENTER_EVENTS)
     ? ecb_mouse_enter_event
     : NULL));

  glfwSetWindowCloseCallback(window,
    (flag_check(events_mask, WINDOW_CLOSE_EVENTS)
     ? ecb_window_close_event
     : NULL));

  glfwSetWindowPosCallback(window,
    (flag_check(events_mask, WINDOW_MOVE_EVENTS)
     ? ecb_window_move_event
     : NULL));

  glfwSetWindowSizeCallback(window,
    (flag_check(events_mask, WINDOW_SIZE_EVENTS)
     ? ecb_window_size_event
     : NULL));

  glfwSetWindowFocusCallback(window,
    (flag_check(events_mask, WINDOW_FOCUS_EVENTS)
     ? ecb_window_focus_event
     : NULL));

  glfwSetWindowIconifyCallback(window,
    (flag_check(events_mask, WINDOW_ICONIFY_EVENTS)
     ? ecb_window_iconify_event
     : NULL));

  #undef flag_check
}



/*******************************************************************************
*                        Event callback implementations                        *
*******************************************************************************/

void event_queue_t::ecb_key_event(GLFWwindow *window, int key, int action, int mods)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, KEY_EVENT, queue->frame_time_};
    event.key = {key, action, mods};
    queue->emit_event(event);
  }
}



void event_queue_t::ecb_mouse_event(GLFWwindow *window, int button, int action, int mods)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, MOUSE_EVENT, queue->frame_time_};
    event.mouse = {button, action, mods};
    queue->emit_event(event);
  }
}



void event_queue_t::ecb_char_event(GLFWwindow *window, unsigned int character)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, CHAR_EVENT, queue->frame_time_};
    event.character = character;
    queue->emit_event(event);
  }
}



void event_queue_t::ecb_mouse_pos_event(GLFWwindow *window, double x, double y)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, MOUSE_MOVE_EVENT, queue->frame_time_};
    event.mouse_pos = {x, y};
    queue->emit_event(event);
  }
}



void event_queue_t::ecb_mouse_scroll_event(GLFWwindow *window, double x, double y)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, MOUSE_SCROLL_EVENT, queue->frame_time_};
    event.scroll = {x, y};
    queue->emit_event(event);
  }
}



void event_queue_t::ecb_mouse_enter_event(GLFWwindow *window, int entered)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, MOUSE_ENTER_EVENT, queue->frame_time_};
    event.entered = (entered == GL_TRUE);
    queue->emit_event(event);
  }
}


void event_queue_t::ecb_window_close_event(GLFWwindow *window)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_CLOSE_EVENT, queue->frame_time_};
    queue->emit_event(event);
  }
  glfwSetWindowShouldClose(window, GL_FALSE);
}



void event_queue_t::ecb_window_move_event(GLFWwindow *window, int x, int y)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_MOVE_EVENT, queue->frame_time_};
    event.window_pos = {x, y};
    queue->emit_event(event);
  }
}



void event_queue_t::ecb_window_size_event(GLFWwindow *window, int width, int height)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_SIZE_EVENT, queue->frame_time_};
    event.window_size = {width, height};
    queue->emit_event(event);
  }
}



void event_queue_t::ecb_window_focus_event(GLFWwindow *window, int focused)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_FOCUS_EVENT, queue->frame_time_};
    event.focused = (focused == GL_TRUE);
    queue->emit_event(event);
  }
}



void event_queue_t::ecb_window_iconify_event(GLFWwindow *window, int iconified)
{
  auto queue = (event_queue_t *)glfwGetWindowUserPointer(window);
  if (queue) {
    event_t event = {EVENT_SENDER_WINDOW, {window}, WINDOW_ICONIFY_EVENT, queue->frame_time_};
    event.iconified = (iconified == GL_TRUE);
    queue->emit_event(event);
  }
}


} // namespace snow
